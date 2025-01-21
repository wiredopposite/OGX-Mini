#include "class/cdc/cdc_device.h"
#include "bsp/board_api.h"

#include "Board/ogxm_log.h"
#include "Descriptors/CDCDev.h"
#include "USBDevice/DeviceDriver/WebApp/WebApp.h"

void WebAppDevice::initialize() 
{
    class_driver_ = 
    {
        .name = TUD_DRV_NAME("WEBAPP"),
        .init = cdcd_init,
        .deinit = cdcd_deinit,
        .reset = cdcd_reset,
        .open = cdcd_open,
        .control_xfer_cb = cdcd_control_xfer_cb,
        .xfer_cb = cdcd_xfer_cb,
        .sof = NULL
    };
}

bool WebAppDevice::read_serial(void* buffer, size_t len, bool block)
{
    if (!block && !tud_cdc_available()) 
    {
        return false;
    }

    uint8_t* buf_ptr = reinterpret_cast<uint8_t*>(buffer);
    size_t total_read = 0;

    while (total_read < len)
    {
        if (!tud_cdc_connected())
        {
            return false;
        }
        tud_task();

        if (tud_cdc_available())
        {
            size_t read = tud_cdc_read(buf_ptr + total_read, len - total_read);
            total_read += read;
        }
    }
    OGXM_LOG("Read %d bytes\n", total_read);
    OGXM_LOG_HEX(buf_ptr, total_read);

    return total_read == len;
}

bool WebAppDevice::write_serial(const void* buffer, size_t len)
{
    const uint8_t* buf_ptr = reinterpret_cast<const uint8_t*>(buffer);
    size_t total_written = 0;

    while (total_written < len)
    {
        if (!tud_cdc_connected())
        {
            return false;
        }
        tud_task();

        if (tud_cdc_write_available())
        {
            size_t written = tud_cdc_write(buf_ptr + total_written, len - total_written);
            total_written += written;
            tud_cdc_write_flush();
        }
    }
    OGXM_LOG("Wrote %d bytes\n", total_written);
    OGXM_LOG_HEX(buf_ptr, total_written);

    return total_written == len;
}

bool WebAppDevice::write_packet(const Packet& packet)
{
    if (!write_serial(&packet, sizeof(Packet)))
    {
        return false;
    }
    return true;
}

bool WebAppDevice::read_packet(Packet& packet, bool block)
{
    if (!read_serial(&packet, sizeof(Packet), block))
    {
        return false;
    }
    return true;
}

//Blocking read
bool WebAppDevice::read_profile(UserProfile& profile)
{
    Packet packet_out;
    uint8_t* profile_data = reinterpret_cast<uint8_t*>(&profile);
    uint8_t current_chunk = 0;
    uint8_t expected_chunks = 0;
    size_t profile_offset = 0;

    while (current_chunk < expected_chunks || expected_chunks == 0)
    {
        if (!read_packet(packet_out, true))
        {
            OGXM_LOG("Failed to read profile packet\n");
            return false;
        }
        if (packet_out.header.packet_id != PacketID::SET_PROFILE)
        {
            OGXM_LOG("Invalid packet ID: %i\n", packet_out.header.packet_id);
            return false;
        }
        if (expected_chunks == 0 && packet_out.header.chunks_total > 0)
        {
            expected_chunks = packet_out.header.chunks_total;
            OGXM_LOG("Expected chunks: %d\n", expected_chunks);
        }
        else if (expected_chunks == 0)
        {
            OGXM_LOG("Invalid chunk count\n");
            return false;
        }
        if (packet_out.header.chunk_len > sizeof(UserProfile) - profile_offset)
        {
            OGXM_LOG("Invalid chunk length\n");
            return false;
        }

        OGXM_LOG("Reading chunk %d\n", current_chunk);
        OGXM_LOG("Chunks IDX: %d\n", packet_out.header.chunk_idx);

        std::memcpy(profile_data + profile_offset, packet_out.data.data(), packet_out.header.chunk_len);
        current_chunk++;
        profile_offset += packet_out.header.chunk_len;
    }
    OGXM_LOG("Profile read successfully\n");
    return true;
}

bool WebAppDevice::write_profile(uint8_t index, const UserProfile& profile, PacketID packet_id)
{
    Packet packet_in;
    const uint8_t* profile_data = reinterpret_cast<const uint8_t*>(&profile);
    uint8_t total_chunks = static_cast<uint8_t>((sizeof(UserProfile) + packet_in.data.size() - 1) / packet_in.data.size());
    uint8_t current_chunk = 0;

    OGXM_LOG("Writing profile\n");

    packet_in.header.packet_id = packet_id;
    packet_in.header.max_gamepads = MAX_GAMEPADS;
    packet_in.header.player_idx = index;
    packet_in.header.profile_id = profile.id;
    packet_in.header.chunks_total = total_chunks;

    while (current_chunk < total_chunks)
    {
        size_t offset = current_chunk * packet_in.data.size();
        size_t remaining_bytes = sizeof(UserProfile) - offset;
        uint8_t current_chunk_len = static_cast<uint8_t>(std::min(packet_in.data.size(), remaining_bytes));

        packet_in.header.chunk_idx = current_chunk;
        packet_in.header.chunk_len = current_chunk_len;

        std::memcpy(packet_in.data.data(), profile_data + offset, packet_in.header.chunk_len);

        if (!write_packet(packet_in))
        {
            OGXM_LOG("Failed to write profile packet\n");
            return false;
        }
        current_chunk++;
    }

    OGXM_LOG("Profile ID %i written successfully\n", profile.id);
    return true;
}

bool WebAppDevice::write_gamepad(uint8_t index, const Gamepad::PadIn& pad_in)
{
    Packet packet_in;
    const uint8_t* pad_in_data = reinterpret_cast<const uint8_t*>(&pad_in);
    const uint8_t total_chunks = static_cast<uint8_t>((sizeof(Gamepad::PadIn) + packet_in.data.size() - 1) / packet_in.data.size());
    uint8_t current_chunk = 0;

    packet_in.header.packet_id = PacketID::SET_GP_IN;
    packet_in.header.max_gamepads = MAX_GAMEPADS;
    packet_in.header.player_idx = index;
    packet_in.header.chunks_total = total_chunks;

    while (current_chunk < total_chunks)
    {
        size_t offset = current_chunk * packet_in.data.size();
        size_t remaining_bytes = sizeof(Gamepad::PadIn) - offset;
        uint8_t current_chunk_len = static_cast<uint8_t>(std::min(packet_in.data.size(), remaining_bytes));

        packet_in.header.chunk_idx = current_chunk;
        packet_in.header.chunk_len = current_chunk_len;

        std::memcpy(packet_in.data.data(), pad_in_data + offset, packet_in.header.chunk_len);

        if (!write_packet(packet_in))
        {
            return false;
        }
        current_chunk++;
    }
    return true;
}

void WebAppDevice::write_error()
{
    Packet packet_in;
    packet_in.header.chunks_total = 1; 
    packet_in.header.packet_id = PacketID::RESP_ERROR;
    write_packet(packet_in);
}

void WebAppDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    if (!tud_cdc_connected())
    {
        return;
    }

    tud_cdc_write_flush();
    bool success = false;   
    static Packet packet_out;

    if (tud_cdc_available())
    {
        OGXM_LOG("Reading packet\n");

        Packet packet_out;
        if (!read_packet(packet_out, true))
        {
            return;
        }

        OGXM_LOG("Received packet with ID: %d\n", packet_out.header.packet_id);
        
        switch (packet_out.header.packet_id)
        {
            case PacketID::GET_PROFILE_BY_ID:
                OGXM_LOG("Getting profile by ID: %i\n", packet_out.header.profile_id);

                profile_ = user_settings_.get_profile_by_id(packet_out.header.profile_id);
                if (!write_profile(0, profile_, PacketID::GET_PROFILE_BY_ID))
                {
                    write_error();
                    return;
                }
                break;

            case PacketID::GET_PROFILE_BY_IDX:
                OGXM_LOG("Getting profile by index: %i\n", packet_out.header.player_idx);

                profile_ = user_settings_.get_profile_by_index(packet_out.header.player_idx);
                if (!write_profile(packet_out.header.player_idx, profile_, PacketID::GET_PROFILE_BY_IDX))
                {
                    write_error();
                    return;
                }
                break;

            case PacketID::SET_PROFILE_START:
                if (!read_profile(profile_))
                {
                    write_error();
                    return;
                }
                if (packet_out.header.device_driver != DeviceDriverType::WEBAPP &&
                    user_settings_.is_valid_driver(packet_out.header.device_driver))
                {
                    success = user_settings_.store_profile_and_driver_type(packet_out.header.device_driver, packet_out.header.player_idx, profile_);
                }
                else
                {
                    success = user_settings_.store_profile(packet_out.header.player_idx, profile_);
                }
                if (!success)
                {
                    write_error();
                    return;
                }
                break;

            default:
                // write_response(PacketID::RESP_ERROR);
                return;
        }
    } 
    else if (gamepad.new_pad_in())
    {
        OGXM_LOG("Writing gamepad input\n");
        Gamepad::PadIn gp_in = gamepad.get_pad_in();
        write_gamepad(idx, gp_in);
    }
}

uint16_t WebAppDevice::get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) 
{
    return reqlen;
}

void WebAppDevice::set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {}

bool WebAppDevice::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) 
{
    return false;
}

const uint16_t * WebAppDevice::get_descriptor_string_cb(uint8_t index, uint16_t langid) 
{
    static uint16_t desc_str[32 + 1];
    size_t char_count = 0;

    switch(index)
    {
        case 0:
            std::memcpy(&desc_str[1], CDCDesc::DESC_STRING[0], 2);
            char_count = 1;
            break;

        case 3:
            char_count = board_usb_get_serial(&desc_str[1], 32);
            break;

        default:
            if (index >= sizeof(CDCDesc::DESC_STRING) / sizeof(CDCDesc::DESC_STRING[0]))
            {
                return nullptr;
            }
            const char *str = reinterpret_cast<const char *>(CDCDesc::DESC_STRING[index]);
            char_count = std::strlen(str);
            const size_t max_count = sizeof(desc_str) / sizeof(desc_str[0]) - 1;    
            if (char_count > max_count)
            {
                char_count = max_count;
            }

            for (size_t i = 0; i < char_count; i++)
            {
                desc_str[1 + i] = str[i];
            }
            break;
    }

    desc_str[0] = static_cast<uint16_t>((TUSB_DESC_STRING << 8) | (2 * char_count + 2));
    return desc_str;
}

const uint8_t * WebAppDevice::get_descriptor_device_cb() 
{
    return reinterpret_cast<const uint8_t*>(&CDCDesc::DESC_DEVICE);
}

const uint8_t * WebAppDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * WebAppDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return CDCDesc::DESC_CONFIG;
}

const uint8_t * WebAppDevice::get_descriptor_device_qualifier_cb() 
{
    return nullptr;
}