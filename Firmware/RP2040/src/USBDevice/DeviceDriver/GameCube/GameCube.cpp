#include <cstring>
#include "pico/flash.h"
#include "USBDevice/DeviceDriver/GameCube/GameCube.h"
#include "USBDevice/DeviceDriver/GameCube/gc_simulator.h"

#ifndef GAMECUBE_DATA_PIN
#define GAMECUBE_DATA_PIN 19
#endif

static GCReport* s_gc_report = nullptr;

void gamecube_core1_entry(void) {
    flash_safe_execute_core_init();  // allow Core0 to perform flash operations (e.g. UserSettings)
    if (s_gc_report == nullptr)
        return;  // driver not initialized; avoid null deref in gc_device_main
    gc_device_main(0, s_gc_report, GAMECUBE_DATA_PIN);
}

namespace {
    constexpr uint8_t GC_A     = 0x01;
    constexpr uint8_t GC_B     = 0x02;
    constexpr uint8_t GC_X     = 0x04;
    constexpr uint8_t GC_Y     = 0x08;
    constexpr uint8_t GC_START = 0x10;
    constexpr uint8_t GC_DPAD_LEFT  = 0x01;
    constexpr uint8_t GC_DPAD_RIGHT  = 0x02;
    constexpr uint8_t GC_DPAD_DOWN  = 0x04;
    constexpr uint8_t GC_DPAD_UP    = 0x08;
    constexpr uint8_t GC_Z     = 0x10;
    constexpr uint8_t GC_R     = 0x20;
    constexpr uint8_t GC_L     = 0x40;
    constexpr uint8_t GC_BYTE2_MSB  = 0x80;
    constexpr uint8_t STICK_CENTER  = 0x80;
}

void GameCubeDevice::initialize() {
    std::memset(&gc_report_, 0, sizeof(gc_report_));
    s_gc_report = &gc_report_;
    gc_report_.lx = STICK_CENTER;
    gc_report_.ly = STICK_CENTER;
    gc_report_.rx = STICK_CENTER;
    gc_report_.ry = STICK_CENTER;
    gc_report_.buttons2 = GC_BYTE2_MSB;
}

void GameCubeDevice::process(const uint8_t idx, Gamepad& gamepad) {
    (void)idx;
    if (!gamepad.new_pad_in())
        return;

    Gamepad::PadIn gp = gamepad.get_pad_in();

    gc_report_.buttons1 = 0;
    if (gp.buttons & Gamepad::BUTTON_A)     gc_report_.buttons1 |= GC_A;
    if (gp.buttons & Gamepad::BUTTON_B)     gc_report_.buttons1 |= GC_B;
    if (gp.buttons & Gamepad::BUTTON_X)     gc_report_.buttons1 |= GC_X;
    if (gp.buttons & Gamepad::BUTTON_Y)     gc_report_.buttons1 |= GC_Y;
    if (gp.buttons & Gamepad::BUTTON_START) gc_report_.buttons1 |= GC_START;

    gc_report_.buttons2 = GC_BYTE2_MSB;
    if (gp.dpad & Gamepad::DPAD_LEFT)  gc_report_.buttons2 |= GC_DPAD_LEFT;
    if (gp.dpad & Gamepad::DPAD_RIGHT) gc_report_.buttons2 |= GC_DPAD_RIGHT;
    if (gp.dpad & Gamepad::DPAD_DOWN)  gc_report_.buttons2 |= GC_DPAD_DOWN;
    if (gp.dpad & Gamepad::DPAD_UP)    gc_report_.buttons2 |= GC_DPAD_UP;
    if (gp.buttons & Gamepad::BUTTON_MISC) gc_report_.buttons2 |= GC_Z;
    if (gp.buttons & Gamepad::BUTTON_RB)   gc_report_.buttons2 |= GC_R;
    if (gp.buttons & Gamepad::BUTTON_LB)   gc_report_.buttons2 |= GC_L;

    gc_report_.l = gp.trigger_l;
    gc_report_.r = gp.trigger_r;

    int32_t lx = (int32_t)gp.joystick_lx * 127 / 32767 + STICK_CENTER;
    int32_t ly = (int32_t)gp.joystick_ly * -127 / 32767 + STICK_CENTER;
    int32_t rx = (int32_t)gp.joystick_rx * 127 / 32767 + STICK_CENTER;
    int32_t ry = (int32_t)gp.joystick_ry * -127 / 32767 + STICK_CENTER;
    if (lx < 0) lx = 0; else if (lx > 255) lx = 255;
    if (ly < 0) ly = 0; else if (ly > 255) ly = 255;
    if (rx < 0) rx = 0; else if (rx > 255) rx = 255;
    if (ry < 0) ry = 0; else if (ry > 255) ry = 255;
    gc_report_.lx = (uint8_t)lx;
    gc_report_.ly = (uint8_t)ly;
    gc_report_.rx = (uint8_t)rx;
    gc_report_.ry = (uint8_t)ry;
}

uint16_t GameCubeDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t req_len) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)req_len;
    return 0;
}
void GameCubeDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t buffer_size) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)buffer_size;
}
bool GameCubeDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
    (void)rhport; (void)stage; (void)request;
    return false;
}
const uint16_t* GameCubeDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)index; (void)langid;
    return nullptr;
}
const uint8_t* GameCubeDevice::get_descriptor_device_cb() { return nullptr; }
const uint8_t* GameCubeDevice::get_hid_descriptor_report_cb(uint8_t itf) {
    (void)itf;
    return nullptr;
}
const uint8_t* GameCubeDevice::get_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return nullptr;
}
const uint8_t* GameCubeDevice::get_descriptor_device_qualifier_cb() { return nullptr; }
