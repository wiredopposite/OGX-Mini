#ifndef _XBOXGOG_SB_DEVICE_H_
#define _XBOXGOG_SB_DEVICE_H_

#include <cstdint>
#include <numeric>

#include "USBDevice/DeviceDriver/DeviceDriver.h"
#include "Descriptors/XboxOG.h"

class XboxOGSBDevice : public DeviceDriver 
{
public:
    struct ButtonMap
    {
        uint16_t gp_mask;
        uint16_t sb_mask;
        uint8_t button_offset;
    };

    void initialize() override;
    void process(const uint8_t idx, Gamepad& gamepad) override;
    uint16_t get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) override;
    void set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) override;
    bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) override;
    const uint16_t* get_descriptor_string_cb(uint8_t index, uint16_t langid) override;
    const uint8_t* get_descriptor_device_cb() override;
    const uint8_t* get_hid_descriptor_report_cb(uint8_t itf)  override;
    const uint8_t* get_descriptor_configuration_cb(uint8_t index) override;
    const uint8_t* get_descriptor_device_qualifier_cb() override;

private:
    static constexpr int16_t DEFAULT_DEADZONE = 7500;
    static constexpr uint16_t DEFAULT_SENSE = 400;

    int32_t vmouse_x_ = XboxOG::SB::AIMING_MID;
    int32_t vmouse_y_ = XboxOG::SB::AIMING_MID;
    uint16_t sensitivity_ = DEFAULT_SENSE;
    uint32_t aim_reset_timer_ = 0;
    bool dpad_reset_ = true;
    

    XboxOG::SB::InReport in_report_;
    XboxOG::SB::InReport prev_in_report_;
    XboxOG::SB::OutReport out_report_;

    static inline bool chatpad_pressed(const Gamepad::ChatpadIn& chatpad, const uint16_t keycode)
    {
        if (std::accumulate(std::begin(chatpad), std::end(chatpad), 0) == 0) 
        {
            return false;
        }
        else if (keycode < 17 && (chatpad[0] & keycode)) 
        {
            return true;
        }
        else if (keycode < 17) 
        {
            return false;
        }
        else if (chatpad[1] == keycode)
        {
            return true;
        }
        else if (chatpad[2] == keycode)
        {
            return true;
        }
        return false;
    }
};

#endif // _XBOXGOG_SB_DEVICE_H_
