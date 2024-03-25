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

void XInputHost::process_xinput_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len)
{   
    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
    xinput_gamepad_t *xinput_data = &xid_itf->pad;

    if (xid_itf->connected && xid_itf->new_pad_data)
    {
        gp.reset_state();

        gp.state.up    = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        gp.state.down  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        gp.state.left  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        gp.state.right = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

        gp.state.a     = (xinput_data->wButtons & XINPUT_GAMEPAD_A) != 0;
        gp.state.b     = (xinput_data->wButtons & XINPUT_GAMEPAD_B) != 0;
        gp.state.x     = (xinput_data->wButtons & XINPUT_GAMEPAD_X) != 0;
        gp.state.y     = (xinput_data->wButtons & XINPUT_GAMEPAD_Y) != 0;

        gp.state.l3    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
        gp.state.r3    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        gp.state.back  = (xinput_data->wButtons & XINPUT_GAMEPAD_BACK) != 0;
        gp.state.start = (xinput_data->wButtons & XINPUT_GAMEPAD_START) != 0;

        gp.state.rb    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        gp.state.lb    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        gp.state.sys   = (xinput_data->wButtons & XINPUT_GAMEPAD_GUIDE) != 0;  
        gp.state.misc  = (xinput_data->wButtons & XINPUT_GAMEPAD_SHARE) != 0; 

        gp.state.lt = xinput_data->bLeftTrigger;
        gp.state.rt = xinput_data->bRightTrigger;

        gp.state.lx = xinput_data->sThumbLX;
        gp.state.ly = xinput_data->sThumbLY;

        gp.state.rx = xinput_data->sThumbRX;
        gp.state.ry = xinput_data->sThumbRY;
    }

    tuh_xinput_receive_report(dev_addr, instance);
}

void XInputHost::process_hid_report(Gamepad& gp, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {}

bool XInputHost::send_fb_data(GamepadOut& gp_out, uint8_t dev_addr, uint8_t instance)
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

    return tuh_xinput_set_rumble(dev_addr, instance, gp_out.state.lrumble, gp_out.state.rrumble, true);
}