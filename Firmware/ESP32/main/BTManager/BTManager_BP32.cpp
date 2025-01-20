#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#include "btstack_stdio_esp32.h"
#include "uni.h"

#include "Board/ogxm_log.h"
#include "BTManager/BTManager.h"
#include "BLEServer/BLEServer.h"

void BTManager::init(int argc, const char** arg_V) 
{
    OGXM_LOG("BP32: Initializing Bluepad32\n");
}

void BTManager::init_complete_cb(void) 
{
    uni_bt_enable_new_connections_unsafe(true);
    // uni_bt_del_keys_unsafe();
    uni_property_dump_all();
}

uni_error_t BTManager::device_discovered_cb(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) 
{
    if (!((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_GAMEPAD))
    {
        return UNI_ERROR_IGNORE_DEVICE;
    }
    return UNI_ERROR_SUCCESS;
}

void BTManager::device_connected_cb(uni_hid_device_t* device) 
{
    OGXM_LOG("BP32: Device connected, addr:  %p, index: %i\n", device, uni_hid_device_get_idx_for_instance(device));
}

void BTManager::device_disconnected_cb(uni_hid_device_t* device) 
{
    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_GAMEPADS || idx < 0)
    {
        return;
    }
    manage_connection(static_cast<uint8_t>(idx), false);
}

uni_error_t BTManager::device_ready_cb(uni_hid_device_t* device) 
{
    int idx = uni_hid_device_get_idx_for_instance(device);
    if (idx >= MAX_GAMEPADS || idx < 0)
    {
        return UNI_ERROR_IGNORE_DEVICE;
    }
    manage_connection(static_cast<uint8_t>(idx), true);
    return UNI_ERROR_SUCCESS;
}

void BTManager::controller_data_cb(uni_hid_device_t* bp_device, uni_controller_t* controller) 
{
    static uni_gamepad_t prev_uni_gps[MAX_GAMEPADS] = {};

    if (controller->klass != UNI_CONTROLLER_CLASS_GAMEPAD)
    {
        return;
    }

    int idx = uni_hid_device_get_idx_for_instance(bp_device);
    uni_gamepad_t *uni_gp = &controller->gamepad;

    if (idx < 0 || idx >= MAX_GAMEPADS || std::memcmp(uni_gp, &prev_uni_gps[idx], sizeof(uni_gamepad_t)) == 0)
    {
        return;
    }

    I2CDriver::PacketIn& packet_in =  devices_[idx].packet_in;
    GamepadMapper& mapper =  devices_[idx].mapper;

    packet_in = I2CDriver::PacketIn();
    packet_in.packet_id = I2CDriver::PacketID::SET_PAD;
    packet_in.index = static_cast<uint8_t>(idx);

    switch (uni_gp->dpad) 
    {
        case DPAD_UP:
            packet_in.dpad = mapper.DPAD_UP;
            break;
        case DPAD_DOWN:
            packet_in.dpad = mapper.DPAD_DOWN;
            break;
        case DPAD_LEFT:
            packet_in.dpad = mapper.DPAD_LEFT;
            break;
        case DPAD_RIGHT:
            packet_in.dpad = mapper.DPAD_RIGHT;
            break;
        case (DPAD_UP | DPAD_RIGHT):
            packet_in.dpad = mapper.DPAD_UP_RIGHT;
            break;
        case (DPAD_DOWN | DPAD_RIGHT):
            packet_in.dpad = mapper.DPAD_DOWN_RIGHT;
            break;
        case (DPAD_DOWN | DPAD_LEFT):
            packet_in.dpad = mapper.DPAD_DOWN_LEFT;
            break;
        case (DPAD_UP | DPAD_LEFT):
            packet_in.dpad = mapper.DPAD_UP_LEFT;
            break;
        default:
            break;
    }

    if (uni_gp->buttons & BUTTON_A) packet_in.buttons |= mapper.BUTTON_A;
    if (uni_gp->buttons & BUTTON_B) packet_in.buttons |= mapper.BUTTON_B;
    if (uni_gp->buttons & BUTTON_X) packet_in.buttons |= mapper.BUTTON_X;
    if (uni_gp->buttons & BUTTON_Y) packet_in.buttons |= mapper.BUTTON_Y;
    if (uni_gp->buttons & BUTTON_SHOULDER_L) packet_in.buttons |= mapper.BUTTON_LB;
    if (uni_gp->buttons & BUTTON_SHOULDER_R) packet_in.buttons |= mapper.BUTTON_RB;
    if (uni_gp->buttons & BUTTON_THUMB_L)    packet_in.buttons |= mapper.BUTTON_L3;
    if (uni_gp->buttons & BUTTON_THUMB_R)    packet_in.buttons |= mapper.BUTTON_R3;
    if (uni_gp->misc_buttons & MISC_BUTTON_BACK)    packet_in.buttons |= mapper.BUTTON_BACK;
    if (uni_gp->misc_buttons & MISC_BUTTON_START)   packet_in.buttons |= mapper.BUTTON_START;
    if (uni_gp->misc_buttons & MISC_BUTTON_SYSTEM)  packet_in.buttons |= mapper.BUTTON_SYS;
    if (uni_gp->misc_buttons & MISC_BUTTON_CAPTURE) packet_in.buttons |= mapper.BUTTON_MISC;

    packet_in.trigger_l = mapper.scale_trigger_l<10>(static_cast<uint16_t>(uni_gp->brake));
    packet_in.trigger_r = mapper.scale_trigger_r<10>(static_cast<uint16_t>(uni_gp->throttle));

    // auto joy_l = mapper.scale_joystick_l<10>(uni_gp->axis_x, uni_gp->axis_y);
    // auto joy_r = mapper.scale_joystick_r<10>(uni_gp->axis_rx, uni_gp->axis_ry);

    // packet_in.joystick_lx = joy_l.first;
    // packet_in.joystick_ly = joy_l.second;
    // packet_in.joystick_rx = joy_r.first;
    // packet_in.joystick_ry = joy_r.second;

    std::tie(packet_in.joystick_lx, packet_in.joystick_ly) = mapper.scale_joystick_l<10>(uni_gp->axis_x, uni_gp->axis_y);
    std::tie(packet_in.joystick_rx, packet_in.joystick_ry) = mapper.scale_joystick_r<10>(uni_gp->axis_rx, uni_gp->axis_ry);

    i2c_driver_.write_packet(I2CDriver::MULTI_SLAVE ? packet_in.index + 1 : 0x01, packet_in);

    std::memcpy(&prev_uni_gps[idx], uni_gp, sizeof(uni_gamepad_t));
}

const uni_property_t* BTManager::get_property_cb(uni_property_idx_t idx) 
{
    return nullptr;
}

void BTManager::oob_event_cb(uni_platform_oob_event_t event, void* data) 
{
    return;
}

uni_platform* BTManager::get_bp32_driver() 
{
    static uni_platform driver = {
        .name = "OGXMiniW",
        .init = 
            [](int argc, const char** argv)
            { get_instance().init(argc, argv); },

        .on_init_complete = 
            [](void)
            { get_instance().init_complete_cb(); },

        .on_device_discovered = 
            [](bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) 
            { return get_instance().device_discovered_cb(addr, name, cod, rssi); },

        .on_device_connected = 
            [](uni_hid_device_t* device) 
            { get_instance().device_connected_cb(device); },

        .on_device_disconnected = 
            [](uni_hid_device_t* device) 
            { get_instance().device_disconnected_cb(device); },

        .on_device_ready = 
            [](uni_hid_device_t* device) 
            { return get_instance().device_ready_cb(device); }, 

        .on_controller_data = 
            [](uni_hid_device_t* device, uni_controller_t* controller) 
            { get_instance().controller_data_cb(device, controller); },

        .get_property = 
            [](uni_property_idx_t idx) 
            {  return get_instance().get_property_cb(idx); },

        .on_oob_event = 
            [](uni_platform_oob_event_t event, void* data) 
            { get_instance().oob_event_cb(event, data); },
    };
    return &driver;
}