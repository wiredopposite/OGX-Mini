#include <cstring>
#include <vector>
#include "pico/stdlib.h"

#include "USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.h"
#include "USBDevice/DeviceDriver/XboxOG/XboxOG_XR.h"

void XboxOGXRDevice::initialize() 
{
    tud_xid::initialize(tud_xid::Type::XREMOTE);
    class_driver_ = *tud_xid::class_driver();

    std::memset(&in_report_, 0, sizeof(XboxOG::XR::InReport));
    in_report_.bLength = sizeof(XboxOG::XR::InReport);
}

void XboxOGXRDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    if (!tud_xid::xremote_rom_available())
    {
        return;
    }

    uint8_t index = tud_xid::get_index_by_type(0, tud_xid::Type::XREMOTE);
    uint32_t time_elapsed = to_ms_since_boot(get_absolute_time()) - ms_timer_;

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }

    if (!tud_xid::send_report_ready(index) || time_elapsed < 64)
    {
        return;
    }

    in_report_.buttonCode = 0;

    uint16_t gp_buttons = gamepad.get_buttons();
    uint8_t gp_dpad = gamepad.get_dpad_buttons();

    if (gp_dpad & Gamepad::DPad::UP)    in_report_.buttonCode |= XboxOG::XR::ButtonCode::UP   ;
    if (gp_dpad & Gamepad::DPad::DOWN)  in_report_.buttonCode |= XboxOG::XR::ButtonCode::DOWN ;
    if (gp_dpad & Gamepad::DPad::LEFT)  in_report_.buttonCode |= XboxOG::XR::ButtonCode::LEFT ;
    if (gp_dpad & Gamepad::DPad::RIGHT) in_report_.buttonCode |= XboxOG::XR::ButtonCode::RIGHT;

    if (gp_buttons & Gamepad::Button::SYS)   in_report_.buttonCode |= XboxOG::XR::ButtonCode::DISPLAY;
    if (gp_buttons & Gamepad::Button::START) in_report_.buttonCode |= XboxOG::XR::ButtonCode::PLAY   ;
    if (gp_buttons & Gamepad::Button::BACK)  in_report_.buttonCode |= XboxOG::XR::ButtonCode::STOP   ;

    if (gp_buttons & Gamepad::Button::L3 && !(gp_buttons & Gamepad::Button::R3)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::TITLE;
    if (gp_buttons & Gamepad::Button::R3 && !(gp_buttons & Gamepad::Button::L3)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::MENU;
    if (gp_buttons & (Gamepad::Button::L3 | Gamepad::Button::R3)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::INFO;

    if (gp_buttons & Gamepad::Button::A) in_report_.buttonCode |= XboxOG::XR::ButtonCode::SELECT ;
    if (gp_buttons & Gamepad::Button::Y) in_report_.buttonCode |= XboxOG::XR::ButtonCode::PAUSE  ;
    if (gp_buttons & Gamepad::Button::X) in_report_.buttonCode |= XboxOG::XR::ButtonCode::DISPLAY;
    if (gp_buttons & Gamepad::Button::B) in_report_.buttonCode |= XboxOG::XR::ButtonCode::BACK   ;

    if (gp_buttons & Gamepad::Button::LB && !(gp_buttons & Gamepad::Button::RB)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::SKIP_MINUS;
    if (gp_buttons & Gamepad::Button::RB && !(gp_buttons & Gamepad::Button::LB)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::SKIP_PLUS ;
    if (gp_buttons & (Gamepad::Button::LB | Gamepad::Button::RB)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::DISPLAY;

    if (gamepad.get_trigger_l().uint8() >= 100) in_report_.buttonCode |= XboxOG::XR::ButtonCode::REVERSE;
    if (gamepad.get_trigger_r().uint8() >= 100) in_report_.buttonCode |= XboxOG::XR::ButtonCode::FORWARD;

    if (in_report_.buttonCode == 0x0000)
    {
        return;
    }

    in_report_.timeElapsed = static_cast<uint16_t>(time_elapsed);
    
    if (tud_xid::send_report(index, reinterpret_cast<uint8_t*>(&in_report_), sizeof(XboxOG::XR::InReport)))
    {
        ms_timer_ = to_ms_since_boot(get_absolute_time());
    }
}

uint16_t XboxOGXRDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    std::memcpy(buffer, &in_report_, sizeof(in_report_));
	return sizeof(XboxOG::XR::InReport);
}

void XboxOGXRDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool XboxOGXRDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return tud_xid::class_driver()->control_xfer_cb(rhport, stage, request);
}

const uint16_t* XboxOGXRDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(XboxOG::GP::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* XboxOGXRDevice::get_descriptor_device_cb() 
{
    return reinterpret_cast<const uint8_t*>(&XboxOG::XR::DEVICE_DESCRIPTORS);
}

const uint8_t* XboxOGXRDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t* XboxOGXRDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return XboxOG::XR::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* XboxOGXRDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}