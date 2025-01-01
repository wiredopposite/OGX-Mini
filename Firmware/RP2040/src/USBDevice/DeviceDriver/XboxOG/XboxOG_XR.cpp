#include <cstring>

#include "Board/board_api.h"
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

    uint32_t time_elapsed = board_api::ms_since_boot() - ms_timer_;
    uint8_t index = tud_xid::get_index_by_type(0, tud_xid::Type::XREMOTE);

    if (index == 0xFF || !gamepad.new_pad_in() || time_elapsed < 64)
    {
        return;
    }

    Gamepad::PadIn gp_in = gamepad.get_pad_in();

    in_report_.buttonCode = 0;

    if (gp_in.dpad & Gamepad::DPAD_UP)    in_report_.buttonCode |= XboxOG::XR::ButtonCode::UP   ;
    if (gp_in.dpad & Gamepad::DPAD_DOWN)  in_report_.buttonCode |= XboxOG::XR::ButtonCode::DOWN ;
    if (gp_in.dpad & Gamepad::DPAD_LEFT)  in_report_.buttonCode |= XboxOG::XR::ButtonCode::LEFT ;
    if (gp_in.dpad & Gamepad::DPAD_RIGHT) in_report_.buttonCode |= XboxOG::XR::ButtonCode::RIGHT;

    if (gp_in.buttons & Gamepad::BUTTON_SYS)   in_report_.buttonCode |= XboxOG::XR::ButtonCode::DISPLAY;
    if (gp_in.buttons & Gamepad::BUTTON_START) in_report_.buttonCode |= XboxOG::XR::ButtonCode::PLAY   ;
    if (gp_in.buttons & Gamepad::BUTTON_BACK)  in_report_.buttonCode |= XboxOG::XR::ButtonCode::STOP   ;

    if (gp_in.buttons & Gamepad::BUTTON_L3 && !(gp_in.buttons & Gamepad::BUTTON_R3)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::TITLE;
    if (gp_in.buttons & Gamepad::BUTTON_R3 && !(gp_in.buttons & Gamepad::BUTTON_L3)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::MENU;
    if (gp_in.buttons & (Gamepad::BUTTON_L3 | Gamepad::BUTTON_R3)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::INFO;

    if (gp_in.buttons & Gamepad::BUTTON_A) in_report_.buttonCode |= XboxOG::XR::ButtonCode::SELECT ;
    if (gp_in.buttons & Gamepad::BUTTON_Y) in_report_.buttonCode |= XboxOG::XR::ButtonCode::PAUSE  ;
    if (gp_in.buttons & Gamepad::BUTTON_X) in_report_.buttonCode |= XboxOG::XR::ButtonCode::DISPLAY;
    if (gp_in.buttons & Gamepad::BUTTON_B) in_report_.buttonCode |= XboxOG::XR::ButtonCode::BACK   ;

    if (gp_in.buttons & Gamepad::BUTTON_LB && !(gp_in.buttons & Gamepad::BUTTON_RB)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::SKIP_MINUS;
    if (gp_in.buttons & Gamepad::BUTTON_RB && !(gp_in.buttons & Gamepad::BUTTON_LB)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::SKIP_PLUS ;
    if (gp_in.buttons & (Gamepad::BUTTON_LB | Gamepad::BUTTON_RB)) in_report_.buttonCode |= XboxOG::XR::ButtonCode::DISPLAY;

    if (gp_in.trigger_l >= 100) in_report_.buttonCode |= XboxOG::XR::ButtonCode::REVERSE;
    if (gp_in.trigger_r >= 100) in_report_.buttonCode |= XboxOG::XR::ButtonCode::FORWARD;

    if (in_report_.buttonCode == 0x0000)
    {
        return;
    }

    in_report_.timeElapsed = static_cast<uint16_t>(time_elapsed);

    if (tud_suspended())
    {
        tud_remote_wakeup();
    }
    if (tud_xid::send_report_ready(index) &&
        tud_xid::send_report(index, reinterpret_cast<uint8_t*>(&in_report_), sizeof(XboxOG::XR::InReport)))
    {
        ms_timer_ = board_api::ms_since_boot();
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