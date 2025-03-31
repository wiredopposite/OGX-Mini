#include <cstring>
#include <string>
#include <algorithm>

#include "att_delayed_response.h"
#include "btstack.h"

#include "BLEServer/BLEServer.h"
#include "UserSettings/UserProfile.h"
#include "UserSettings/UserSettings.h"
#include "TaskQueue/TaskQueue.h"

namespace BLEServer {

static constexpr uint16_t PACKET_LEN_MAX = 20;

namespace Handle {
    static constexpr uint16_t FW_VERSION    = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789020_01_VALUE_HANDLE;
    static constexpr uint16_t FW_NAME       = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789021_01_VALUE_HANDLE;

    static constexpr uint16_t SETUP_READ  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789030_01_VALUE_HANDLE;
    static constexpr uint16_t SETUP_WRITE = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789031_01_VALUE_HANDLE;
    static constexpr uint16_t GET_SETUP   = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789032_01_VALUE_HANDLE;

    static constexpr uint16_t PROFILE  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789040_01_VALUE_HANDLE;

    static constexpr uint16_t GAMEPAD  = ATT_CHARACTERISTIC_12345678_1234_1234_1234_123456789050_01_VALUE_HANDLE;
}

namespace ADV {
    // Flags general discoverable, BR/EDR not supported
    static const uint8_t FLAGS[]    = { 0x02, 0x01, 0x06 };
    static const uint8_t NAME_TYPE  = 0x09;

    #pragma pack(push, 1)
    struct Data {
        uint8_t flags[sizeof(FLAGS)];
        uint8_t name_len;
        uint8_t name_type;
        uint8_t name[sizeof(FIRMWARE_NAME) - 1];

        Data() {
            std::memcpy(flags, FLAGS, sizeof(flags));
            name_len = sizeof(FIRMWARE_NAME);
            name_type = NAME_TYPE;
            std::string fw_name = FIRMWARE_NAME;
            std::memcpy(name, fw_name.c_str(), std::min(sizeof(name), fw_name.size()));
        }
    };
    static_assert(sizeof(Data) == 5 + sizeof(FIRMWARE_NAME) - 1, "BLEServer::ADV::Data struct size mismatch");
    #pragma pack(pop)
}

#pragma pack(push, 1)
struct SetupPacket {
    DeviceDriverType device_type{DeviceDriverType::NONE};
    uint8_t max_gamepads{MAX_GAMEPADS};
    uint8_t player_idx{0};
    uint8_t profile_id{0};
};
static_assert(sizeof(SetupPacket) == 4, "BLEServer::SetupPacket struct size mismatch");
#pragma pack(pop)

class ProfileReader {
public:
    ProfileReader() = default;
    ~ProfileReader() = default;

    void set_setup_packet(const SetupPacket& setup_packet) {
        setup_packet_ = setup_packet;
        current_offset_ = 0;
    }

    const SetupPacket& get_setup_packet() const {
        return setup_packet_;
    }

    uint16_t get_xfer_len() {
        return static_cast<uint16_t>(std::min(static_cast<size_t>(PACKET_LEN_MAX), sizeof(UserProfile) - current_offset_));
    }

    uint16_t get_profile_data(uint8_t* buffer, uint16_t buffer_len) {
        size_t copy_len = get_xfer_len();
        if (!buffer || buffer_len < copy_len) {
            return 0;
        }
        if (current_offset_ == 0 && !set_profile()) {
            return 0;
        }
        std::memcpy(buffer, reinterpret_cast<uint8_t*>(&profile_) + current_offset_, copy_len);
        current_offset_ += copy_len;
        if (current_offset_ >= sizeof(UserProfile)) {
            current_offset_ = 0;
        }
        return copy_len;
    }

private:
    SetupPacket setup_packet_;
    UserProfile profile_;
    size_t current_offset_ = 0;

    bool set_profile() {
        if (setup_packet_.profile_id == 0xFF) {
            if (setup_packet_.player_idx >= UserSettings::MAX_PROFILES) {
                return false;
            }
            profile_ = UserSettings::get_instance().get_profile_by_index(setup_packet_.player_idx);
        } else {
            if (setup_packet_.profile_id > UserSettings::MAX_PROFILES) {
                return false;
            }
            profile_ = UserSettings::get_instance().get_profile_by_id(setup_packet_.profile_id);
        }
        return true;
    }
};

class ProfileWriter
{
public:
    ProfileWriter() = default;
    ~ProfileWriter() = default;

    void set_setup_packet(const SetupPacket& setup_packet) {
        setup_packet_ = setup_packet;
        current_offset_ = 0;
    }

    const SetupPacket& get_setup_packet() const {
        return setup_packet_;
    }

    uint16_t get_xfer_len() {
        return static_cast<uint16_t>(std::min(static_cast<size_t>(PACKET_LEN_MAX), sizeof(UserProfile) - current_offset_));
    }

    size_t set_profile_data(const uint8_t* buffer, uint16_t buffer_len) {
        size_t copy_len = get_xfer_len();
        if (!buffer || buffer_len < copy_len) {
            return 0;
        }

        std::memcpy(reinterpret_cast<uint8_t*>(&profile_) + current_offset_, buffer, copy_len);

        current_offset_ += copy_len;
        size_t ret = current_offset_;

        if (current_offset_ >= sizeof(UserProfile)) {
            current_offset_ = 0;
        }
        return ret;
    }

    bool commit_profile() {
        bool success = false;
        if (setup_packet_.device_type != DeviceDriverType::NONE) {
            success = TaskQueue::Core0::queue_delayed_task(TaskQueue::Core0::get_new_task_id(), 1000, false,
                [driver_type = setup_packet_.device_type, profile = profile_, index = setup_packet_.player_idx]
                {
                    UserSettings::get_instance().store_profile_and_driver_type(driver_type, index, profile);
                });
        } else {
            success = TaskQueue::Core0::queue_delayed_task(TaskQueue::Core0::get_new_task_id(), 1000, false,
                [index = setup_packet_.player_idx, profile = profile_]
                {
                    UserSettings::get_instance().store_profile(index, profile);
                });
        }
        return success;
    }

private:
    SetupPacket setup_packet_;
    UserProfile profile_;
    size_t current_offset_ = 0;
};

std::array<Gamepad*, MAX_GAMEPADS> gamepads_;
ProfileReader profile_reader_;
ProfileWriter profile_writer_;

static int verify_write(const uint16_t buffer_size, const uint16_t expected_size) {
    if (buffer_size != expected_size) {
        return ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
    }
    return 0;
}

static void disconnect_client_cb(btstack_timer_source_t *ts) {
    hci_con_handle_t connection_handle = *static_cast<hci_con_handle_t*>(ts->context);
    hci_send_cmd(&hci_disconnect, connection_handle);
    delete static_cast<hci_con_handle_t*>(ts->context);
}

static void queue_disconnect(hci_con_handle_t connection_handle, uint32_t dealy_ms) {
    static btstack_timer_source_t disconnect_timer;

    hci_con_handle_t* connection_handle_ptr = new hci_con_handle_t(connection_handle);

    disconnect_timer.process = disconnect_client_cb;
    disconnect_timer.context = connection_handle_ptr;

    btstack_run_loop_set_timer(&disconnect_timer, dealy_ms);
    btstack_run_loop_add_timer(&disconnect_timer);
}

static uint16_t att_read_callback(  hci_con_handle_t connection_handle,
                                    uint16_t att_handle,
                                    uint16_t offset,
                                    uint8_t *buffer,
                                    uint16_t buffer_size) {
    std::string fw_version;
    std::string fw_name;
    Gamepad::PadIn pad_in;

    switch (att_handle) {
        case Handle::FW_VERSION:
            fw_version = FIRMWARE_VERSION;
            if (buffer)  {
                std::memcpy(buffer, reinterpret_cast<const uint8_t*>(fw_version.c_str()), fw_version.size());
            }
            return static_cast<uint16_t>(fw_version.size());

        case Handle::FW_NAME:
            fw_name = FIRMWARE_NAME;
            if (buffer) {
                std::memcpy(buffer, reinterpret_cast<const uint8_t*>(fw_name.c_str()), fw_name.size());;
            }
            return static_cast<uint16_t>(fw_name.size());

        case Handle::GET_SETUP:
            if (buffer) {
                buffer[0] = static_cast<uint8_t>(UserSettings::get_instance().get_current_driver());
                buffer[1] = MAX_GAMEPADS;
                buffer[2] = 0;
                buffer[3] = UserSettings::get_instance().get_active_profile_id(0);
            }
            return static_cast<uint16_t>(sizeof(SetupPacket));

        case Handle::PROFILE:
            if (buffer) {
                return profile_reader_.get_profile_data(buffer, buffer_size);
            }
            return profile_reader_.get_xfer_len();

        case Handle::GAMEPAD:
            if (buffer) {
                pad_in = gamepads_.front()->get_pad_in();
                std::memcpy(buffer, &pad_in, sizeof(Gamepad::PadIn));
            }
            return static_cast<uint16_t>(sizeof(Gamepad::PadIn));

        default:
            break;
    }
    return 0;
}

static int att_write_callback(  hci_con_handle_t connection_handle,
                                uint16_t att_handle,
                                uint16_t transaction_mode,
                                uint16_t offset,
                                uint8_t *buffer,
                                uint16_t buffer_size) {
    int ret = 0;

    switch (att_handle) {
        case Handle::SETUP_READ:
            if ((ret = verify_write(buffer_size, sizeof(SetupPacket))) != 0) {
                break;
            }
            profile_reader_.set_setup_packet(*reinterpret_cast<SetupPacket*>(buffer));
            break;

        case Handle::SETUP_WRITE:
            if ((ret = verify_write(buffer_size, sizeof(SetupPacket))) != 0) {
                break;
            }
            profile_writer_.set_setup_packet(*reinterpret_cast<SetupPacket*>(buffer));
            break;

        case Handle::PROFILE:
            if ((ret = verify_write(buffer_size, profile_writer_.get_xfer_len())) != 0) {
                break;
            }
            if (profile_writer_.set_profile_data(buffer, buffer_size) == sizeof(UserProfile)) {
                queue_disconnect(connection_handle, 500);
                profile_writer_.commit_profile();
            }
            break;

        default:
            break;
    }
    return ret;
}

void init_server(Gamepad(&gamepads)[MAX_GAMEPADS]) {
    for (uint8_t i = 0; i < MAX_GAMEPADS; i++) {
        gamepads_[i] = &gamepads[i];
    }

    UserSettings::get_instance().initialize_flash();

    // setup ATT server
    att_server_init(profile_data, att_read_callback, att_write_callback);

    // setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;

    bd_addr_t null_addr;
    std::memset(null_addr, 0, sizeof(null_addr));

    static ADV::Data adv_data = ADV::Data();

    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(static_cast<uint8_t>(sizeof(adv_data)), reinterpret_cast<uint8_t*>(&adv_data));
    gap_advertisements_enable(1);
}

} // namespace BLEServer