#include <cstring>

#include "USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.h"
#include "USBDevice/DeviceDriver/XboxOG/XboxOG_GP.h"
#include "Gamepad/Gamepad.h"
#if defined(CONFIG_OGXM_DEBUG)
#include <string>
#include "Board/ogxm_log.h"
#endif

void XboxOGDevice::initialize()
{
#if defined(CONFIG_OGXM_DEBUG)
    OGXM_LOG("XboxOG: initialize\n");
#endif
    tud_xid::initialize(tud_xid::Type::GAMEPAD);
    class_driver_ = *tud_xid::class_driver();

    std::memset(&in_report_, 0, sizeof(XboxOG::GP::InReport));
    in_report_.report_len = sizeof(XboxOG::GP::InReport);
#if defined(CONFIG_OGXM_DEBUG)
    OGXM_LOG("XboxOG: init done\n");
#endif
}

void XboxOGDevice::process(const uint8_t idx, Gamepad& gamepad)
{
    /* Build/send only when new input (match Team-Resurgent timing). Guide/Guide+Start combos for IGR/shutdown kept. */
#if defined(CONFIG_OGXM_DEBUG)
    static uint32_t s_process_ticks = 0;
    static uint32_t s_send_count = 0;
    static uint32_t s_new_pad_in_count = 0;
    static uint32_t s_last_new_pad_in_tick = 0;
    static uint32_t s_ready_false_count = 0;  // had new_pad_in but send_report_ready(0) was false
    s_process_ticks++;
#endif
    if (gamepad.new_pad_in())
    {
#if defined(CONFIG_OGXM_DEBUG)
        s_new_pad_in_count++;
        s_last_new_pad_in_tick = s_process_ticks;
#endif
        std::memset(&in_report_.buttons, 0, 8);
        Gamepad::PadIn gp_in = gamepad.get_pad_in();

        // Guide only = IGR (LT+RT+Start+Back).
        // Shutdown = LT+RT+Back+White only (no Start on the wire — extra Start breaks many BIOSes).
        // Guide+Menu (Start): works on PS5 / some pads. Xbox BT often drops Menu while Guide is held;
        // use Guide+View (Back) instead on Xbox controllers.
        bool guide_pressed = (gp_in.buttons & Gamepad::BUTTON_SYS) != 0;
        bool start_pressed = (gp_in.buttons & Gamepad::BUTTON_START) != 0;
        bool view_pressed = (gp_in.buttons & Gamepad::BUTTON_BACK) != 0;
        const bool shutdown_combo =
            guide_pressed && (start_pressed || view_pressed);
        if (shutdown_combo)
        {
            gp_in.trigger_l = gamepad.scale_trigger_l(0xFF);
            gp_in.trigger_r = gamepad.scale_trigger_r(0xFF);
            gp_in.buttons |= Gamepad::BUTTON_BACK;
            gp_in.buttons |= Gamepad::BUTTON_LB;  // White
            gp_in.buttons &= ~Gamepad::BUTTON_START;  // chord must not include Start
        }
        else if (guide_pressed)
        {
            gp_in.trigger_l = gamepad.scale_trigger_l(0xFF);
            gp_in.trigger_r = gamepad.scale_trigger_r(0xFF);
            gp_in.buttons |= Gamepad::BUTTON_START;
            gp_in.buttons |= Gamepad::BUTTON_BACK;
        }

        switch (gp_in.dpad)
        {
            case Gamepad::DPAD_UP:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP;
                break;
            case Gamepad::DPAD_DOWN:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN;
                break;
            case Gamepad::DPAD_LEFT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_LEFT;
                break;
            case Gamepad::DPAD_RIGHT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_UP_LEFT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP | XboxOG::GP::Buttons::DPAD_LEFT;
                break;
            case Gamepad::DPAD_UP_RIGHT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_UP | XboxOG::GP::Buttons::DPAD_RIGHT;
                break;
            case Gamepad::DPAD_DOWN_LEFT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN | XboxOG::GP::Buttons::DPAD_LEFT;
                break;
            case Gamepad::DPAD_DOWN_RIGHT:
                in_report_.buttons = XboxOG::GP::Buttons::DPAD_DOWN | XboxOG::GP::Buttons::DPAD_RIGHT;
                break;
            default:
                break;
        }

        if (gp_in.buttons & Gamepad::BUTTON_BACK)     in_report_.buttons |= XboxOG::GP::Buttons::BACK;
        if (gp_in.buttons & Gamepad::BUTTON_START)    in_report_.buttons |= XboxOG::GP::Buttons::START;
        if (gp_in.buttons & Gamepad::BUTTON_L3)       in_report_.buttons |= XboxOG::GP::Buttons::L3;
        if (gp_in.buttons & Gamepad::BUTTON_R3)       in_report_.buttons |= XboxOG::GP::Buttons::R3;

        if (gamepad.analog_enabled())
        {
            in_report_.a = gp_in.analog[Gamepad::ANALOG_OFF_A];
            in_report_.b = gp_in.analog[Gamepad::ANALOG_OFF_B];
            in_report_.x = gp_in.analog[Gamepad::ANALOG_OFF_X];
            in_report_.y = gp_in.analog[Gamepad::ANALOG_OFF_Y];
            in_report_.white = gp_in.analog[Gamepad::ANALOG_OFF_LB];
            in_report_.black = gp_in.analog[Gamepad::ANALOG_OFF_RB];
        }
        else
        {
            if (gp_in.buttons & Gamepad::BUTTON_X)    in_report_.x = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_A)    in_report_.a = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_Y)    in_report_.y = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_B)    in_report_.b = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_LB)   in_report_.white = 0xFF;
            if (gp_in.buttons & Gamepad::BUTTON_RB)   in_report_.black = 0xFF;
        }

        in_report_.trigger_l = gp_in.trigger_l;
        in_report_.trigger_r = gp_in.trigger_r;

        in_report_.joystick_lx = gp_in.joystick_lx;
        in_report_.joystick_ly = Range::invert(gp_in.joystick_ly);
        in_report_.joystick_rx = gp_in.joystick_rx;
        in_report_.joystick_ry = Range::invert(gp_in.joystick_ry);

        if (tud_suspended())
        {
#if defined(CONFIG_OGXM_DEBUG)
            OGXM_LOG("XboxOG: tud_remote_wakeup\n");
#endif
            tud_remote_wakeup();
        }
        bool ready = tud_xid::send_report_ready(0);
#if defined(CONFIG_OGXM_DEBUG)
        if (ready)
            s_send_count++;
        else
        {
            s_ready_false_count++;
            if (s_ready_false_count <= 3 || s_ready_false_count % 2000 == 0)
                OGXM_LOG("XboxOG: new_pad_in but send_report_ready=false (count=" + std::to_string(s_ready_false_count) + ")\n");
        }
#endif
        if (ready)
        {
            tud_xid::send_report(0, reinterpret_cast<uint8_t*>(&in_report_), sizeof(XboxOG::GP::InReport));
        }
    }

#if defined(CONFIG_OGXM_DEBUG)
    // Periodic status every 10000 process ticks to see why it might stop responding
    if (s_process_ticks > 0 && s_process_ticks % 10000 == 0)
    {
        uint32_t ticks_since_input = s_process_ticks - s_last_new_pad_in_tick;
        OGXM_LOG("XboxOG: tick=" + std::to_string(s_process_ticks) +
                 " new_pad_in=" + std::to_string(s_new_pad_in_count) +
                 " sent=" + std::to_string(s_send_count) +
                 " ready_false=" + std::to_string(s_ready_false_count) +
                 " ticks_since_input=" + std::to_string(ticks_since_input) + "\n");
        if (ticks_since_input > 20000)
            OGXM_LOG("XboxOG: WARNING no new input for " + std::to_string(ticks_since_input) + " ticks (controller/driver may be idle or not sending)\n");
    }
#endif

    if (tud_xid::receive_report(0, reinterpret_cast<uint8_t*>(&out_report_), sizeof(XboxOG::GP::OutReport)))
    {
#if defined(CONFIG_OGXM_DEBUG)
        static uint32_t s_rumble_recv_count = 0;
        s_rumble_recv_count++;
        if (s_rumble_recv_count <= 3 || s_rumble_recv_count % 2000 == 0)
            OGXM_LOG("XboxOG: rumble recv #" + std::to_string(s_rumble_recv_count) + "\n");
#endif
        Gamepad::PadOut gp_out;
        gp_out.rumble_l = Scale::uint16_to_uint8(out_report_.rumble_l);
        gp_out.rumble_r = Scale::uint16_to_uint8(out_report_.rumble_r);
        gamepad.set_pad_out(gp_out);
    }
}

uint16_t XboxOGDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
#if defined(CONFIG_OGXM_DEBUG)
    static uint32_t s_get_report_count = 0;
    static uint32_t s_last_logged_get_report = 0;
    s_get_report_count++;
    // Log first 5, then every 2000 so we see if Xbox stops polling (get_report_cb stops being called)
    if (s_get_report_count <= 5 || s_get_report_count - s_last_logged_get_report >= 2000)
    {
        OGXM_LOG("XboxOG: get_report_cb #" + std::to_string(s_get_report_count) + " (Xbox polling)\n");
        s_last_logged_get_report = s_get_report_count;
    }
#endif
    std::memcpy(buffer, &in_report_, sizeof(in_report_));
	return sizeof(XboxOG::GP::InReport);
}

void XboxOGDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool XboxOGDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return tud_xid::class_driver()->control_xfer_cb(rhport, stage, request);
}

const uint16_t* XboxOGDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
	const char *value = reinterpret_cast<const char*>(XboxOG::GP::STRING_DESCRIPTORS[index]);
	return get_string_descriptor(value, index);
}

const uint8_t* XboxOGDevice::get_descriptor_device_cb() 
{
    return reinterpret_cast<const uint8_t*>(&XboxOG::GP::DEVICE_DESCRIPTORS);
}

const uint8_t* XboxOGDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t* XboxOGDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return XboxOG::GP::CONFIGURATION_DESCRIPTORS;
}

const uint8_t* XboxOGDevice::get_descriptor_device_qualifier_cb() 
{
	return nullptr;
}