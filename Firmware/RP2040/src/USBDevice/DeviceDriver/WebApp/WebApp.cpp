#include "class/cdc/cdc_device.h"
#include "bsp/board_api.h"

#include "Descriptors/CDCDev.h"
#include "USBDevice/DeviceDriver/WebApp/WebApp.h"

void WebAppDevice::initialize() 
{
    class_driver_ = 
    {
    #if CFG_TUSB_DEBUG >= 2
        .name = "WEBAPP",
    #endif
        .init = cdcd_init,
        .reset = cdcd_reset,
        .open = cdcd_open,
        .control_xfer_cb = cdcd_control_xfer_cb,
        .xfer_cb = cdcd_xfer_cb,
        .sof = NULL
    };
}

void WebAppDevice::process(const uint8_t idx, Gamepad& gamepad) 
{
    if (!tud_cdc_available() || !tud_cdc_connected())
    {
        return;
    }

    tud_cdc_write_flush();
    tud_cdc_read(reinterpret_cast<void*>(&in_report_), sizeof(Report));
    tud_cdc_read_flush();

    bool success = false;

    switch (in_report_.report_id)
    {
        case ReportID::INIT_READ:
            in_report_.input_mode = static_cast<uint8_t>(driver_type_);
            in_report_.player_idx = 0;
            in_report_.report_id = ReportID::RESP_OK;
            in_report_.max_gamepads = MAX_GAMEPADS;

            in_report_.profile.id = user_settings_.get_active_profile_id(in_report_.player_idx);
            in_report_.profile = user_settings_.get_profile_by_id(in_report_.profile.id);

            tud_cdc_write(reinterpret_cast<const void*>(&in_report_), sizeof(Report));
            tud_cdc_write_flush();
            break;

        case ReportID::READ_PROFILE:
            in_report_.input_mode = static_cast<uint8_t>(driver_type_);
            in_report_.profile = user_settings_.get_profile_by_id(in_report_.profile.id);
            in_report_.report_id = ReportID::RESP_OK;

            tud_cdc_write(reinterpret_cast<const void*>(&in_report_), sizeof(Report));
            tud_cdc_write_flush();
            break;

        case ReportID::WRITE_PROFILE:
            if (in_report_.input_mode != static_cast<uint8_t>(driver_type_) && in_report_.input_mode != 0)
            {
                success = user_settings_.store_profile_and_driver_type_safe(static_cast<DeviceDriver::Type>(in_report_.input_mode), in_report_.player_idx, in_report_.profile);
            }
            else
            {
                success = user_settings_.store_profile_safe(in_report_.player_idx, in_report_.profile);
            }
            if (!success)
            {
                in_report_.report_id = ReportID::RESP_ERROR;
                tud_cdc_write(reinterpret_cast<const void*>(&in_report_), sizeof(Report));
                tud_cdc_write_flush();
            }
            break;
            
        default:
            return;
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
	size_t chr_count;

	switch ( index ) 
	{
		case CDCDesc::STRID_LANGID:
			std::memcpy(&CDCDesc::_desc_str[1], CDCDesc::STRING_DESCRIPTORS[0], 2);
			chr_count = 1;
			break;

		case CDCDesc::STRID_SERIAL:
			chr_count = board_usb_get_serial(CDCDesc::_desc_str + 1, 32);
			break;

		default:
			if ( !(index < sizeof(CDCDesc::STRING_DESCRIPTORS) / sizeof(CDCDesc::STRING_DESCRIPTORS[0])) ) 
            {
                return NULL;
            }

			const char *str = CDCDesc::STRING_DESCRIPTORS[index];

			chr_count = strlen(str);
			size_t const max_count = sizeof(CDCDesc::_desc_str) / sizeof(CDCDesc::_desc_str[0]) - 1;

			if ( chr_count > max_count ) 
            {
                chr_count = max_count;
            }

			for ( size_t i = 0; i < chr_count; i++ ) 
            {
				CDCDesc::_desc_str[1 + i] = str[i];
			}
			break;
	}

	CDCDesc::_desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

	return CDCDesc::_desc_str;
}

const uint8_t * WebAppDevice::get_descriptor_device_cb() 
{
    return reinterpret_cast<const uint8_t*>(&CDCDesc::DEVICE_DESCRIPTORS);
}

const uint8_t * WebAppDevice::get_hid_descriptor_report_cb(uint8_t itf) 
{
    return nullptr;
}

const uint8_t * WebAppDevice::get_descriptor_configuration_cb(uint8_t index) 
{
    return CDCDesc::CONFIGURATION_DESCRIPTORS;
}

const uint8_t * WebAppDevice::get_descriptor_device_qualifier_cb() 
{
    return nullptr;
}