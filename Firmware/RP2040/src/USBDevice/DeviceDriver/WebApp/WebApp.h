#ifndef _WEBAAPP_DEVICE_H_
#define _WEBAAPP_DEVICE_H_

#include <array>

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
    enum class PacketID : uint8_t
    {
        NONE = 0,
        GET_PROFILE_BY_ID = 0x50,
        GET_PROFILE_BY_IDX = 0x55,
        SET_PROFILE_START = 0x60,
        SET_PROFILE = 0x61,
        SET_GP_IN = 0x80,
        SET_GP_OUT = 0x81,
        RESP_ERROR = 0xFF
    };
    
    #pragma pack(push, 1)
    struct PacketHeader
    {
        uint8_t packet_len{64};
        PacketID packet_id{PacketID::NONE};
        DeviceDriverType device_driver{DeviceDriverType::WEBAPP};
        uint8_t max_gamepads{MAX_GAMEPADS};
        uint8_t player_idx{0};
        uint8_t profile_id{0};
        uint8_t chunks_total{0};
        uint8_t chunk_idx{0};
        uint8_t chunk_len{0};
    };
    static_assert(sizeof(PacketHeader) == 9, "WebApp report size mismatch");

    struct Packet
    {
        PacketHeader header;
        std::array<uint8_t, 64 - sizeof(PacketHeader)> data{0};
    };
    static_assert(sizeof(Packet) == 64, "WebApp report size mismatch");
    #pragma pack(pop)

    UserSettings& user_settings_{UserSettings::get_instance()};
    UserProfile profile_;

    bool read_profile(UserProfile& profile);
    bool read_serial(void* buffer, size_t len, bool block);
    bool read_packet(Packet& packet, bool block);
    bool write_serial(const void* buffer, size_t len);
    bool write_packet(const Packet& packet);
    bool write_profile(uint8_t index, const UserProfile& profile, PacketID packet_id);
    bool write_gamepad(uint8_t index, const Gamepad::PadIn& pad_in);
    void write_error();  
};

#endif // _WEBAAPP_DEVICE_H_
