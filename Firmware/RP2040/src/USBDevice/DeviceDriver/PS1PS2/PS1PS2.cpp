#include <cstring>
#include "pico/multicore.h"
#include "USBDevice/DeviceDriver/PS1PS2/PS1PS2.h"
#include "USBDevice/DeviceDriver/PS1PS2/psx_simulator.h"

namespace {
    constexpr uint8_t PSX_SELECT = 0x01;
    constexpr uint8_t PSX_L3     = 0x02;
    constexpr uint8_t PSX_R3     = 0x04;
    constexpr uint8_t PSX_START  = 0x08;
    constexpr uint8_t PSX_UP     = 0x10;
    constexpr uint8_t PSX_RIGHT  = 0x20;
    constexpr uint8_t PSX_DOWN   = 0x40;
    constexpr uint8_t PSX_LEFT   = 0x80;
    constexpr uint8_t PSX_L2     = 0x01;
    constexpr uint8_t PSX_R2     = 0x02;
    constexpr uint8_t PSX_L1     = 0x04;
    constexpr uint8_t PSX_R1     = 0x08;
    constexpr uint8_t PSX_TRI    = 0x10;
    constexpr uint8_t PSX_CIR    = 0x20;
    constexpr uint8_t PSX_CROSS  = 0x40;
    constexpr uint8_t PSX_SQU    = 0x80;
    constexpr uint8_t STICK_CENTER = 0x80;
}

void PS1PS2Device::core1_restart_cb() {
    multicore_reset_core1();
    multicore_launch_core1(psx_device_main);
}

void PS1PS2Device::initialize() {
    std::memset(&psx_report_, 0, sizeof(psx_report_));
    /* PS2 protocol is active-low: 0xFF = released, 0 = pressed. Default to all released. */
    psx_report_.buttons1 = 0xFF;
    psx_report_.buttons2 = 0xFF;
    psx_report_.lx = STICK_CENTER;
    psx_report_.ly = STICK_CENTER;
    psx_report_.rx = STICK_CENTER;
    psx_report_.ry = STICK_CENTER;
    psx_device_init(0, &psx_report_, &PS1PS2Device::core1_restart_cb);
}

void PS1PS2Device::process(const uint8_t idx, Gamepad& gamepad) {
    (void)idx;
    /* Always refresh report from gamepad so Core1 (psx_device_main) sees current/last state when it reads inputState. */
    Gamepad::PadIn gp = gamepad.get_pad_in();

    /* Active-low: start all released (0xFF), clear bit when pressed */
    psx_report_.buttons1 = 0xFF;
    if (gp.dpad & Gamepad::DPAD_UP)    psx_report_.buttons1 &= ~PSX_UP;
    if (gp.dpad & Gamepad::DPAD_RIGHT) psx_report_.buttons1 &= ~PSX_RIGHT;
    if (gp.dpad & Gamepad::DPAD_DOWN)  psx_report_.buttons1 &= ~PSX_DOWN;
    if (gp.dpad & Gamepad::DPAD_LEFT)  psx_report_.buttons1 &= ~PSX_LEFT;
    if (gp.buttons & Gamepad::BUTTON_BACK)  psx_report_.buttons1 &= ~PSX_SELECT;
    if (gp.buttons & Gamepad::BUTTON_L3)    psx_report_.buttons1 &= ~PSX_L3;
    if (gp.buttons & Gamepad::BUTTON_R3)    psx_report_.buttons1 &= ~PSX_R3;
    if (gp.buttons & Gamepad::BUTTON_START) psx_report_.buttons1 &= ~PSX_START;

    psx_report_.buttons2 = 0xFF;
    if (gp.buttons & Gamepad::BUTTON_LB) psx_report_.buttons2 &= ~PSX_L1;
    if (gp.buttons & Gamepad::BUTTON_RB) psx_report_.buttons2 &= ~PSX_R1;
    if (gp.buttons & Gamepad::BUTTON_Y)  psx_report_.buttons2 &= ~PSX_TRI;
    if (gp.buttons & Gamepad::BUTTON_B)  psx_report_.buttons2 &= ~PSX_CIR;
    if (gp.buttons & Gamepad::BUTTON_A)  psx_report_.buttons2 &= ~PSX_CROSS;
    if (gp.buttons & Gamepad::BUTTON_X)  psx_report_.buttons2 &= ~PSX_SQU;
    if (gp.trigger_l > 0) psx_report_.buttons2 &= ~PSX_L2;
    if (gp.trigger_r > 0) psx_report_.buttons2 &= ~PSX_R2;

    psx_report_.l2 = gp.trigger_l;
    psx_report_.r2 = gp.trigger_r;

    int32_t lx = static_cast<int32_t>(gp.joystick_lx) * 127 / 32767 + STICK_CENTER;
    int32_t ly = static_cast<int32_t>(gp.joystick_ly) * 127 / 32767 + STICK_CENTER;  /* PS2 left stick Y: up = lower value */
    int32_t rx = static_cast<int32_t>(gp.joystick_rx) * 127 / 32767 + STICK_CENTER;
    int32_t ry = static_cast<int32_t>(gp.joystick_ry) * -127 / 32767 + STICK_CENTER;
    if (lx < 0) lx = 0; else if (lx > 255) lx = 255;
    if (ly < 0) ly = 0; else if (ly > 255) ly = 255;
    if (rx < 0) rx = 0; else if (rx > 255) rx = 255;
    if (ry < 0) ry = 0; else if (ry > 255) ry = 255;
    psx_report_.lx = static_cast<uint8_t>(lx);
    psx_report_.ly = static_cast<uint8_t>(ly);
    psx_report_.rx = static_cast<uint8_t>(rx);
    psx_report_.ry = static_cast<uint8_t>(ry);

    /* Home only = IGR (L1+L2+R1+R2+Start+Select); Home+Start = shutdown (L1+L2+R1+R2+L3+R3) */
    bool home_pressed = (gp.buttons & Gamepad::BUTTON_SYS) != 0;
    bool start_pressed = (gp.buttons & Gamepad::BUTTON_START) != 0;
    if (home_pressed && start_pressed) {
        psx_report_.buttons1 &= ~(PSX_L3 | PSX_R3);
        psx_report_.buttons2 &= ~(PSX_L1 | PSX_L2 | PSX_R1 | PSX_R2);
        psx_report_.l2 = 0xFF;
        psx_report_.r2 = 0xFF;
    } else if (home_pressed) {
        psx_report_.buttons1 &= ~(PSX_SELECT | PSX_START);
        psx_report_.buttons2 &= ~(PSX_L1 | PSX_L2 | PSX_R1 | PSX_R2);
        psx_report_.l2 = 0xFF;
        psx_report_.r2 = 0xFF;
    }
}

uint16_t PS1PS2Device::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t req_len) {
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)req_len;
    return 0;
}

void PS1PS2Device::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t buffer_size) {
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)buffer_size;
}

bool PS1PS2Device::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
    (void)rhport;
    (void)stage;
    (void)request;
    return false;
}

const uint16_t* PS1PS2Device::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)index;
    (void)langid;
    return nullptr;
}

const uint8_t* PS1PS2Device::get_descriptor_device_cb() {
    return nullptr;
}

const uint8_t* PS1PS2Device::get_hid_descriptor_report_cb(uint8_t itf) {
    (void)itf;
    return nullptr;
}

const uint8_t* PS1PS2Device::get_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return nullptr;
}

const uint8_t* PS1PS2Device::get_descriptor_device_qualifier_cb() {
    return nullptr;
}
