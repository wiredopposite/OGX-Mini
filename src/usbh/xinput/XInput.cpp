#include <stdint.h>
#include "pico/stdlib.h"

#include "usbh/xinput/XInput.h"

void XInputHost::init(uint8_t player_id, uint8_t dev_addr, uint8_t instance) 
{
    xinput.player_id = player_id;
    tuh_xinput_receive_report(dev_addr, instance);
}

void XInputHost::set_leds(uint8_t dev_addr, uint8_t instance)
{
    tuh_xinput_set_led(dev_addr, instance, 0, true);
    xinput.leds_set = tuh_xinput_set_led(dev_addr, instance, xinput.player_id, true);
}

void XInputHost::process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len)
{   
    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
    xinput_gamepad_t *xinput_data = &xid_itf->pad;

    if (xid_itf->connected && xid_itf->new_pad_data)
    {
        gamepad.reset_pad();

        gamepad.buttons.up    = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        gamepad.buttons.down  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        gamepad.buttons.left  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        gamepad.buttons.right = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

        gamepad.buttons.a     = (xinput_data->wButtons & XINPUT_GAMEPAD_A) != 0;
        gamepad.buttons.b     = (xinput_data->wButtons & XINPUT_GAMEPAD_B) != 0;
        gamepad.buttons.x     = (xinput_data->wButtons & XINPUT_GAMEPAD_X) != 0;
        gamepad.buttons.y     = (xinput_data->wButtons & XINPUT_GAMEPAD_Y) != 0;

        gamepad.buttons.l3    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
        gamepad.buttons.r3    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        gamepad.buttons.back  = (xinput_data->wButtons & XINPUT_GAMEPAD_BACK) != 0;
        gamepad.buttons.start = (xinput_data->wButtons & XINPUT_GAMEPAD_START) != 0;

        gamepad.buttons.rb    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        gamepad.buttons.lb    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        gamepad.buttons.sys   = (xinput_data->wButtons & XINPUT_GAMEPAD_GUIDE) != 0;  
        gamepad.buttons.misc  = (xinput_data->wButtons & XINPUT_GAMEPAD_SHARE) != 0; 

        gamepad.triggers.l = xinput_data->bLeftTrigger;
        gamepad.triggers.r = xinput_data->bRightTrigger;

        gamepad.joysticks.lx = xinput_data->sThumbLX;
        gamepad.joysticks.ly = xinput_data->sThumbLY;

        gamepad.joysticks.rx = xinput_data->sThumbRX;
        gamepad.joysticks.ry = xinput_data->sThumbRY;
    }

    tuh_xinput_receive_report(dev_addr, instance);
}

void XInputHost::process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {}

void XInputHost::hid_get_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len) {}

bool XInputHost::send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance)
{
    static int report_num = 0;

    if (!xinput.leds_set || report_num == 0)
    {
        report_num++;
        set_leds(dev_addr, instance);
        return false;
    }

    report_num++;

    if (report_num >= 50) // send led cmd every so often incase wireless 360 controller disconnects/reconnects
    {
        report_num = 0;
    }

    return tuh_xinput_set_rumble(dev_addr, instance, gamepad.rumble.l, gamepad.rumble.r, true);
}