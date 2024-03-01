#include <stdlib.h>
#include <stdarg.h>

#include "tusb.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "usbh/tusb_xinput/xinput_host.h"
#include "Gamepad.h"

static bool gamepad_mounted = false;
uint8_t connected_dev_addr = 0;
int8_t connected_instance = -1;

bool xinput_gamepad_mounted()
{
    return gamepad_mounted;
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
    gamepad_mounted = true;
    connected_dev_addr = dev_addr;
    connected_instance = instance;

    // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
    // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
    if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false)
    {
        tuh_xinput_receive_report(dev_addr, instance);
        return;
    }

    tuh_xinput_set_led(dev_addr, instance, 0, true);
    tuh_xinput_set_led(dev_addr, instance, 1, true);
    tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
    tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    if (gamepad_mounted && connected_dev_addr == dev_addr && connected_instance == instance)
    {
        gamepad_mounted = false;
    }
}

void Gamepad::update_gamepad_state_from_xinput(const xinput_gamepad_t* xinput_data) 
{
    reset_state();

    state.up    = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
    state.down  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
    state.left  = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
    state.right = (xinput_data->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
    
    state.a     = (xinput_data->wButtons & XINPUT_GAMEPAD_A) != 0;
    state.b     = (xinput_data->wButtons & XINPUT_GAMEPAD_B) != 0;
    state.x     = (xinput_data->wButtons & XINPUT_GAMEPAD_X) != 0;
    state.y     = (xinput_data->wButtons & XINPUT_GAMEPAD_Y) != 0;
    
    state.l3    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
    state.r3    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
    state.back  = (xinput_data->wButtons & XINPUT_GAMEPAD_BACK) != 0;
    state.start = (xinput_data->wButtons & XINPUT_GAMEPAD_START) != 0;
    
    state.rb    = (xinput_data->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
    state.lb    = (xinput_data->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    state.sys   = (xinput_data->wButtons & XINPUT_GAMEPAD_GUIDE) != 0;   

    state.lt = xinput_data->bLeftTrigger;
    state.rt = xinput_data->bRightTrigger;

    state.lx = xinput_data->sThumbLX;
    state.ly = xinput_data->sThumbLY;

    state.rx = xinput_data->sThumbRX;
    state.ry = xinput_data->sThumbRY;
}

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len)
{
    xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
    xinput_gamepad_t *p = &xid_itf->pad;

    if (xid_itf->connected && xid_itf->new_pad_data)
    {
        gamepad.update_gamepad_state_from_xinput(p);
    }

    tuh_xinput_receive_report(dev_addr, instance);
}

bool send_fb_data_to_xinput_gamepad()
{
    bool rumble_sent = tuh_xinput_set_rumble(connected_dev_addr, connected_instance, gamepadOut.out_state.lrumble, gamepadOut.out_state.rrumble, true);
    
    return rumble_sent;
}