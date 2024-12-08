#ifndef _WEBAAPP_DEVICE_H_
#define _WEBAAPP_DEVICE_H_

#include "USBDevice/DeviceDriver/DeviceDriver.h"
#include "UserSettings/UserSettings.h"
#include "UserSettings/UserProfile.h"

class WebAppDevice : public DeviceDriver 
{
public:
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
    struct ReportID
    {
        static constexpr uint8_t INIT_READ = 0x88;
        static constexpr uint8_t READ_PROFILE = 0x01;
        static constexpr uint8_t WRITE_PROFILE = 0x02;
        static constexpr uint8_t RESP_OK = 0x10;
        static constexpr uint8_t RESP_ERROR = 0x11;
    };

    #pragma pack(push, 1)
    struct Report
    {
        uint8_t report_id{0};
        uint8_t input_mode{0};
        uint8_t max_gamepads{MAX_GAMEPADS};
        uint8_t player_idx{0};
        UserProfile profile{UserProfile()};
    };
    static_assert(sizeof(Report) == 50, "WebApp report size mismatch");
    #pragma pack(pop)

    UserSettings user_settings_{UserSettings()};
    Report in_report_{Report()};
    DeviceDriver::Type driver_type_{DeviceDriver::Type::WEBAPP};
};

#endif // _WEBAAPP_DEVICE_H_
