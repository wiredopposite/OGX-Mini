#pragma once

#ifndef _SWITCHPRO_H_
#define _SWITCHPRO_H_

#include <stdint.h>

#include "usbd/descriptors/SwitchProDescriptors.h"
#include "usbh/GPHostDriver.h"

const usb_vid_pid_t switch_pro_devices[] = 
{
    {0x057E, 0x2009} // Switch Pro
};

struct SwitchProState
{
    uint8_t player_id = {0};
    bool handshake_sent {false};
    bool timeout_disabled {false};
    bool full_report_enabled {false};
    bool led_set {false};
    bool led_home_set {false};
    bool imu_enabled {false};
    bool commands_sent {false};
    uint8_t output_sequence_counter {0};
};

class SwitchPro : public GPHostDriver
{
    public:
        virtual void init(uint8_t player_id, uint8_t dev_addr, uint8_t instance);
        virtual void process_hid_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
        virtual void process_xinput_report(Gamepad& gamepad, uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* report, uint16_t len);
        virtual bool send_fb_data(const Gamepad& gamepad, uint8_t dev_addr, uint8_t instance);
    private:
        SwitchProState switch_pro;
        void send_handshake(uint8_t dev_addr, uint8_t instance);
        void disable_timeout(uint8_t dev_addr, uint8_t instance);
        uint8_t get_output_sequence_counter();
        int16_t normalize_axes(uint16_t value);
        void update_gamepad(Gamepad& gp, const SwitchProReport* switch_pro_data);
};

#endif // _SWITCHPRO_H_