/* Dreamcast Maple Bus output (device). USB/BT gamepad -> adapter -> Dreamcast. */

#include "USBDevice/DeviceDriver/Dreamcast/Dreamcast.h"
#include "Gamepad/Gamepad.h"
#include "USBHost/GPIOHost/dreamcast_host/MapleBus.h"
#include "USBHost/GPIOHost/dreamcast_host/MaplePacket.hpp"
#include "USBHost/GPIOHost/dreamcast_host/dreamcast_constants.h"
#include "USBHost/GPIOHost/dreamcast_host/dreamcast_structures.h"
#include "USBHost/GPIOHost/dreamcast_host/maple_config.h"
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/sync.h>
#include <pico/flash.h>
#include <cstring>

static bool s_core1_device_mode = false;
static controller_condition_t s_device_condition;
static mutex_t s_condition_mutex;

/* Standard controller device info (28 words). Layout from BlueRetro / Maple spec. */
#define DC_ID_CTRL    0x00000001
#define DC_DESC_CTRL  0x000F06FE
#define DC_PWR_CTRL   0xAE01F401

static const uint8_t s_ctrl_area_dir_name[32] = {
    0x72, 0x44, 0x00, 0xFF, 0x63, 0x6D, 0x61, 0x65, 0x20, 0x74, 0x73, 0x61, 0x74, 0x6E, 0x6F, 0x43,
    0x6C, 0x6C, 0x6F, 0x72, 0x20, 0x20, 0x72, 0x65, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
};
static const uint8_t s_brand[32] = {
    'O', 'G', 'X', '-', 'M', 'i', 'n', 'i', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
};

static void fill_device_info_payload(uint32_t* out) {
    out[0] = DC_ID_CTRL;
    out[1] = DC_DESC_CTRL;
    out[2] = 0u;
    out[3] = 0u;
    memcpy(&out[4], s_ctrl_area_dir_name, 32);
    memcpy(&out[12], s_brand, 32);
    for (int i = 20; i < 27; i++) out[i] = 0u;
    out[27] = DC_PWR_CTRL;
}

void dreamcast_set_core1_device_mode(bool enable) {
    s_core1_device_mode = enable;
}

static void pad_in_to_controller_condition(const Gamepad::PadIn& pad, controller_condition_t& cond) {
    memset(&cond, 0, sizeof(cond));
    cond.l = pad.trigger_l;
    cond.r = pad.trigger_r;
    cond.a = (pad.buttons & Gamepad::BUTTON_A) ? 0 : 1;
    cond.b = (pad.buttons & Gamepad::BUTTON_B) ? 0 : 1;
    cond.x = (pad.buttons & Gamepad::BUTTON_X) ? 0 : 1;
    cond.y = (pad.buttons & Gamepad::BUTTON_Y) ? 0 : 1;
    cond.start = (pad.buttons & Gamepad::BUTTON_START) ? 0 : 1;
    cond.c = (pad.buttons & Gamepad::BUTTON_RB) ? 0 : 1;
    cond.z = (pad.buttons & Gamepad::BUTTON_LB) ? 0 : 1;
    cond.d = (pad.buttons & Gamepad::BUTTON_MISC) ? 0 : 1;
    cond.up   = (pad.dpad & Gamepad::DPAD_UP)    ? 0 : 1;
    cond.down = (pad.dpad & Gamepad::DPAD_DOWN)  ? 0 : 1;
    cond.left = (pad.dpad & Gamepad::DPAD_LEFT)  ? 0 : 1;
    cond.right= (pad.dpad & Gamepad::DPAD_RIGHT) ? 0 : 1;
    cond.upb = cond.downb = cond.leftb = cond.rightb = 1;
    /* Sticks: int16 -32767..32767 -> 0..255, 128 center */
    auto stick16_to_8 = [](int16_t v) -> uint8_t {
        int32_t x = (int32_t)v * 127 / 32767 + 128;
        if (x < 0) x = 0;
        if (x > 255) x = 255;
        return (uint8_t)x;
    };
    cond.lAnalogLR = stick16_to_8(pad.joystick_lx);
    cond.lAnalogUD = stick16_to_8(pad.joystick_ly);
    cond.rAnalogLR = stick16_to_8(pad.joystick_rx);
    cond.rAnalogUD = stick16_to_8(pad.joystick_ry);
}

void DreamcastDevice::initialize() {
    mutex_init(&s_condition_mutex);
    memset(&s_device_condition, 0, sizeof(s_device_condition));
    s_device_condition.up = s_device_condition.down = s_device_condition.left = s_device_condition.right = 1;
    s_device_condition.a = s_device_condition.b = s_device_condition.c = s_device_condition.x = 1;
    s_device_condition.y = s_device_condition.z = s_device_condition.d = s_device_condition.start = 1;
    s_device_condition.upb = s_device_condition.downb = s_device_condition.leftb = s_device_condition.rightb = 1;
    s_device_condition.lAnalogLR = s_device_condition.lAnalogUD = 128;
    s_device_condition.rAnalogLR = s_device_condition.rAnalogUD = 128;
}

void DreamcastDevice::process(const uint8_t idx, Gamepad& gamepad) {
    if (idx != 0) return;
    Gamepad::PadIn pad = gamepad.get_pad_in();
    controller_condition_t cond;
    pad_in_to_controller_condition(pad, cond);
    mutex_enter_blocking(&s_condition_mutex);
    memcpy(&s_device_condition, &cond, sizeof(s_device_condition));
    mutex_exit(&s_condition_mutex);
}

void dreamcast_core1_entry(void) {
    flash_safe_execute_core_init();  // allow Core0 to perform flash operations (e.g. UserSettings)
    if (!s_core1_device_mode) {
        while (true) sleep_ms(1);
        return;
    }
    const uint32_t pinA = 10;
    MapleBus bus(pinA, -1, true);
    MaplePacket reqPacket, respPacket;
    respPacket.payloadByteOrder = MaplePacket::ByteOrder::HOST;

    while (true) {
        if (!bus.startRead(0, MaplePacket::ByteOrder::HOST))
            continue;
        uint64_t deadline = time_us_64() + MAPLE_RESPONSE_TIMEOUT_US + 2000;
        MapleBus::Status st;
        while (time_us_64() < deadline) {
            st = bus.processEvents(time_us_64());
            if (st.phase == MapleBus::Phase::READ_COMPLETE || st.phase == MapleBus::Phase::READ_FAILED)
                break;
            sleep_us(20);
        }
        if (st.phase != MapleBus::Phase::READ_COMPLETE || !st.readBuffer || st.readBufferLen < 1)
            continue;

        uint32_t frameWord = st.readBuffer[0];
        uint8_t cmd = MaplePacket::Frame::getFrameCommand(frameWord, st.rxByteOrder);
        uint8_t recipient = MaplePacket::Frame::getFrameRecipientAddr(frameWord, st.rxByteOrder);
        uint8_t sender = MaplePacket::Frame::getFrameSenderAddr(frameWord, st.rxByteOrder);

        if (cmd == COMMAND_DEVICE_INFO_REQUEST) {
            respPacket.setFrame(COMMAND_RESPONSE_DEVICE_INFO, MAPLE_HOST_ADDR, MAPLE_CTRL_ADDR, 28);
            fill_device_info_payload(respPacket.payload);
            respPacket.payloadCount = 28;
        } else if (cmd == COMMAND_GET_CONDITION) {
            mutex_enter_blocking(&s_condition_mutex);
            uint32_t condWords[2];
            memcpy(condWords, &s_device_condition, sizeof(condWords));
            mutex_exit(&s_condition_mutex);
            respPacket.setFrame(COMMAND_RESPONSE_DATA_XFER, MAPLE_HOST_ADDR, MAPLE_CTRL_ADDR, 3);
            respPacket.payload[0] = DEVICE_FN_CONTROLLER;
            respPacket.payload[1] = condWords[0];
            respPacket.payload[2] = condWords[1];
            respPacket.payloadCount = 3;
        } else {
            continue;
        }

        if (!bus.write(respPacket, false))
            continue;
        deadline = time_us_64() + 5000;
        while (time_us_64() < deadline && bus.isBusy()) {
            bus.processEvents(time_us_64());
            sleep_us(50);
        }
    }
}

uint16_t DreamcastDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t req_len) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)req_len;
    return 0;
}
void DreamcastDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t buffer_size) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)buffer_size;
}
bool DreamcastDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
    (void)rhport; (void)stage; (void)request;
    return false;
}
const uint16_t* DreamcastDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)index; (void)langid;
    return nullptr;
}
const uint8_t* DreamcastDevice::get_descriptor_device_cb() {
    return nullptr;
}
const uint8_t* DreamcastDevice::get_hid_descriptor_report_cb(uint8_t itf) {
    (void)itf;
    return nullptr;
}
const uint8_t* DreamcastDevice::get_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return nullptr;
}
const uint8_t* DreamcastDevice::get_descriptor_device_qualifier_cb() {
    return nullptr;
}
