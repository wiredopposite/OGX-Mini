/* N64 controller output (device). Any USB/BT gamepad -> adapter -> N64 console. Ported from pdaxrom/usb2n64-adapter. */

#include <cstring>
#include "pico/flash.h"
#include "USBDevice/DeviceDriver/N64/N64.h"
#include "Gamepad/Gamepad.h"

#ifndef N64_DATA_PIN
#define N64_DATA_PIN 19
#endif

void n64_core1_entry(void) {
    flash_safe_execute_core_init();  // allow Core0 to perform flash operations (e.g. UserSettings)
    n64_device_main(N64_DATA_PIN);
}

namespace {
    /* N64 report format matches host input: byte0 A,B,Z,START,D-pad; byte1 L,R,C; byte2/3 joy_x, joy_y (signed, 0 center). */
    constexpr uint8_t N64_A     = 0x80;
    constexpr uint8_t N64_B     = 0x40;
    constexpr uint8_t N64_Z     = 0x20;
    constexpr uint8_t N64_START = 0x10;
    constexpr uint8_t N64_R     = 0x10;
    constexpr uint8_t N64_L     = 0x20;
    constexpr uint8_t N64_C_UP   = 0x08;
    constexpr uint8_t N64_C_DOWN = 0x04;
    constexpr uint8_t N64_C_LEFT = 0x02;
    constexpr uint8_t N64_C_RIGHT = 0x01;
}

void N64Device::initialize() {
    N64Report r = {};
    r.buttons1 = 0;
    r.buttons2 = 0;
    r.joy_x = 0;
    r.joy_y = 0;
    n64_device_set_report(&r);
}

void N64Device::process(const uint8_t idx, Gamepad& gamepad) {
    (void)idx;
    if (!gamepad.new_pad_in())
        return;

    Gamepad::PadIn gp = gamepad.get_pad_in();
    N64Report r = {};
    uint8_t dpad = 0;
    if (gp.dpad & Gamepad::DPAD_UP)    dpad |= 0x08;
    if (gp.dpad & Gamepad::DPAD_DOWN)  dpad |= 0x04;
    if (gp.dpad & Gamepad::DPAD_LEFT)  dpad |= 0x02;
    if (gp.dpad & Gamepad::DPAD_RIGHT) dpad |= 0x01;
    r.buttons1 = dpad;
    if (gp.buttons & Gamepad::BUTTON_A)     r.buttons1 |= N64_A;
    if (gp.buttons & Gamepad::BUTTON_B)     r.buttons1 |= N64_B;
    if (gp.buttons & Gamepad::BUTTON_MISC)  r.buttons1 |= N64_Z;
    if (gp.buttons & Gamepad::BUTTON_START) r.buttons1 |= N64_START;

    r.buttons2 = 0;
    if (gp.buttons & Gamepad::BUTTON_LB) r.buttons2 |= N64_L;
    if (gp.buttons & Gamepad::BUTTON_RB) r.buttons2 |= N64_R;
    if (gp.joystick_ry < -4000) r.buttons2 |= N64_C_UP;
    if (gp.joystick_ry > 4000)  r.buttons2 |= N64_C_DOWN;
    if (gp.joystick_rx < -4000) r.buttons2 |= N64_C_LEFT;
    if (gp.joystick_rx > 4000)  r.buttons2 |= N64_C_RIGHT;

    int32_t jx = (int32_t)gp.joystick_lx * 127 / 32767;
    int32_t jy = (int32_t)gp.joystick_ly * -127 / 32767;
    if (jx < -128) jx = -128; else if (jx > 127) jx = 127;
    if (jy < -128) jy = -128; else if (jy > 127) jy = 127;
    r.joy_x = (int8_t)jx;
    r.joy_y = (int8_t)jy;

    n64_device_set_report(&r);
}

uint16_t N64Device::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t req_len) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)req_len;
    return 0;
}
void N64Device::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t buffer_size) {
    (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)buffer_size;
}
bool N64Device::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
    (void)rhport; (void)stage; (void)request;
    return false;
}
const uint16_t* N64Device::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)index; (void)langid;
    return nullptr;
}
const uint8_t* N64Device::get_descriptor_device_cb() { return nullptr; }
const uint8_t* N64Device::get_hid_descriptor_report_cb(uint8_t itf) {
    (void)itf;
    return nullptr;
}
const uint8_t* N64Device::get_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return nullptr;
}
const uint8_t* N64Device::get_descriptor_device_qualifier_cb() { return nullptr; }
