#include <stdbool.h>
#include <string.h>
#include <pico/rand.h>
#include "usbd/usbd.h"

#include "log/log.h"
#include "gamepad/range.h"
#include "usb/device/device.h"
#include "usb/descriptors/ps3.h"
#include "usb/device/device_private.h"

#define PS3_EF_OFFSET 6U

typedef struct {
    ps3_sixaxis_report_in_t report_in;
    ps3_bt_info_t           bt_info;
    bool                    reports_enabled;
    uint8_t                 idle_rate;
    uint8_t                 protocol;
    uint8_t                 motion_calib_byte;
    gamepad_pad_t           gp_pad;
    gamepad_rumble_t        gp_rumble;
    uint8_t                 bank[2][0x100] __attribute__((aligned(4)));
    uint8_t                 bank_select;
    uint8_t                 bank_address;
    uint8_t                 bank_req_len;
    uint8_t                 host_mac_addr[6] __attribute__((aligned(4)));
} ps3_state_t;
_Static_assert(sizeof(ps3_state_t) <= USBD_STATUS_BUF_SIZE, "PS3 state size exceeds buffer size");

// static const uint8_t PS3_DEFAULT_BT_INFO[] = {
//     0xFF, 0xFF,
//     0x00, 0x20, 0x40, 0xCE, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00
// };

static const ps3_bt_info_t PS3_DEFAULT_BT_INFO = {
    .report_id = PS3_REQ_FEATURE_REPORT_ID_BT_INFO,
    .unk0 = { 0xFF, 0xFF, 0x00 },
    .bdaddr = { 0x20, 0x40, 0xCE, 0x00, 0x00, 0x00 },
    .unk1 = { 0x00, 0x03 },
    .serial = PS3_BT_INFO_SERIAL,
    .pcb_rev = PS3_BT_INFO_PCB_REV
};

// static const uint8_t PS3_DEFAULT_DS3_INFO[] = {
//     0x01, 0x04, 0x00, 0x0b, 0x0c, 0x01, 0x02, 0x18, 
//     0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11, 0x12,
//     0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02,
//     0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04, 0x04,
//     0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01, 0x02,
//     0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };
// 00 01 04 01 03 0c 01 02 18 18 18 18 09 0a 10 11
// 12 13 00 00 00 00 04 00 02 02 02 02 00 00 00 04
// 04 04 04 00 00 03 00 01 02 00 00 17 00 00 00 00
// 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

static const ps3_info_t PS3_DEFAULT_DS3_INFO = {
    .reserved0 = 0x00,
    .report_id = PS3_REQ_FEATURE_REPORT_ID_DS3_INFO,
    .model = PS3_INFO_MODEL_DS3,
    .unk1 = 0x01,
    .fw_version = PS3_INFO_DEFAULT_FW,
    .unk3 = { 0x01, 0x02 },
    .stick_mid = {
        .lx = 0x18,
        .ly = 0x18,
        .rx = 0x18,
        .ry = 0x18
    },
    .calibration = { 
        0x09, 0x0A, 0x10, 0x11, 0x12, 0x13, 0x00, 0x00 
    },
    .unk4 = { 0x00, 0x00 },
    .lx = { .deadzone = 4, .gain = 0 },
    .ly = { .deadzone = 2, .gain = 2 },
    .rx = { .deadzone = 2, .gain = 2 },
    .ry = { .deadzone = 0, .gain = 0 },
    .unk5 = {
        0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x03, 
        0x00, 0x01, 0x02, 0x00, 0x00, 0x17, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00,
    }
};

// calibration data
// static const uint8_t PS3_DEFAULT_MOTION_CALIB[] = {
//     0xef, 0x04, 0x00, 0x0b, 0x03, 0x01, 0xa0, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff,
//     0x01, 0xff, 0x01, 0xff, 0x01, 0xff, 0x01, 0xff,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
// };

// 0xef, 0x04, 0x01, 0x03, 0x03, 0x01, 0xf0, 0x00, 
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 0x12, 0x18, 0x01, 0x01, 0x07, 0x12, 0x00, 0x00, 
// 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
// 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

static const ps3_motion_calib_t PS3_DEFAULT_MOTION_CALIB = {
    .report_id = PS3_REQ_FEATURE_REPORT_ID_MOTION_CALIB,
    .unk0 = { 0x04, 0x00, 0x0b, 0x03, 0x01, },
    .save_byte = 0xf0, 
    .unk1 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    .accel_x = { .bias = 0x1812, .gain = 0x0101 },
    .accel_y = { .bias = 0x1207, .gain = 0x0000 },
    .accel_z = { .bias = 0x0000, .gain = 0x0101 },
    .gyro_z  = { .bias = 0x0101, .gain = 0x0101 },
    .unk2 = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    }
};

// // // unknown
// static const uint8_t PS3_DEFAULT_BT_PAIRING[] = {
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // host address - must match 0xf2
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };

// unknown
// static const uint8_t FEATURE_0xF7[] = {
//     0x02, 0x01, 0xf8, 0x02, 0xe2, 0x01, 0x05, 0xff,
//     0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };
static const uint8_t PS3_UNK_FEATURE_F7[] = {
    0x02, 0x01, 0xfb, 0x02, 0xe2, 0x01, 0xef, 0xff, 
    0x14, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x12, 0x18, 0x01, 0x01, 0x07, 0x12, 0x00, 
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// // unknown
// static const uint8_t FEATURE_0xF8[] = {
//     0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
// };
static const uint8_t PS3_UNK_FEATURE_F8[] = {
    0x00, 0x02, 0x00, 0x00, 0xe2, 0x01, 0xef, 0xff, 
    0x14, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x12, 0x18, 0x01, 0x01, 0x07, 0x12, 0x00, 
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t PS3_UNK_FEATURE_F9[] = {
    0x03, 0x50, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 
    0x00, 0x2a, 0xff, 0x27, 0x10, 0x32, 0x32, 0xff, 
    0x27, 0x10, 0x32, 0x32, 0xff, 0x27, 0x10, 0x32, 
    0x32, 0xff, 0x27, 0x10, 0x32, 0x32, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static ps3_state_t* ps3_state[USBD_DEVICES_MAX] = { NULL };

static void ps3_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void ps3_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool ps3_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &PS3_DESC_DEVICE, 
                                   sizeof(PS3_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &PS3_DESC_CONFIG, 
                                   sizeof(PS3_DESC_CONFIG), NULL);
    case USB_DTYPE_STRING:
        {
        uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (USB_DESC_INDEX(req->wValue) < ARRAY_SIZE(PS3_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, PS3_DESC_STRING[idx], 
                                       PS3_DESC_STRING[idx]->bLength, NULL);
        }
        }
        break;
    case USB_DTYPE_HID_REPORT:
        return usbd_send_ctrl_resp(handle, PS3_DESC_REPORT, 
                                   sizeof(PS3_DESC_REPORT), NULL);
    case USB_DTYPE_HID:
        return usbd_send_ctrl_resp(handle, &PS3_DESC_CONFIG.hid, 
                                   sizeof(PS3_DESC_CONFIG.hid), NULL);
    default:
        break;
    }
    return false;
}

static bool ps3_handle_set_report(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    const uint8_t report_id = req->wValue & 0xFF;
    const uint8_t report_type = req->wValue >> 8;
    ps3_state_t* ps3 = ps3_state[handle->port];
    ogxm_logd("PS3 Set Report: ID=0x%02X, Type=0x%02X, Length=%d\n", report_id, report_type, req->wLength);

    switch (report_type) {
    case USB_REQ_HID_REPORT_TYPE_OUTPUT:
        if ((report_id == PS3_REPORT_ID_OUT) && (req->wLength > 0)) {
            if (req->wLength >= sizeof(ps3_report_out_t)) {
                const ps3_report_out_t* report_out = (const ps3_report_out_t*)req->data;
                ps3->gp_rumble.l = report_out->rumble.l_force;
                ps3->gp_rumble.r = report_out->rumble.r_on ? 0xFFU : 0x00U;
                ps3->gp_rumble.l_duration = report_out->rumble.l_duration;
                ps3->gp_rumble.r_duration = report_out->rumble.r_duration;
                usb_device_rumble_cb(handle, &ps3->gp_rumble);
            }
        }
        break;
    case USB_REQ_HID_REPORT_TYPE_FEATURE:
        switch (report_id) {
        case PS3_REQ_FEATURE_REPORT_ID_COMMAND:
            {
            const ps3_cmd_header_t* cmd = (const ps3_cmd_header_t*)req->data;
            switch (cmd->command) {
            case PS3_COMMAND_INPUT_ENABLE:
                ps3->reports_enabled = true;
                break;
            case PS3_COMMAND_RESTART:
            case PS3_COMMAND_INPUT_DISABLE:
                ps3->reports_enabled = false;
                break;
            case PS3_COMMAND_SENSORS_ENABLE:
                break;
            default:
                break;
            }
            }
            break;
        case PS3_REQ_FEATURE_REPORT_ID_MOTION_CALIB:
            {
            const ps3_motion_calib_t* calib = (const ps3_motion_calib_t*)req->data;
            ps3->motion_calib_byte = calib->save_byte;
            }
            break;
        case PS3_REQ_FEATURE_REPORT_ID_STORAGE:
            {
            ps3_cmd_storage_t* cmd = (ps3_cmd_storage_t*)req->data;
            if (cmd->bank >= 2) {
                return false; // Invalid bank
            }
            switch (cmd->command) {
            case PS3_STORAGE_CMD_READ:
                ogxm_logd("PS3 Read Storage: Bank=%d, Address=0x%02X\n", 
                       cmd->bank, cmd->address);
                ps3->bank_select = cmd->bank;
                ps3->bank_address = cmd->address;
                ps3->bank_req_len = req->wLength;
                break;
            case PS3_STORAGE_CMD_WRITE:
                ogxm_logd("PS3 Write Storage: Bank=%d, Address=0x%02X, Length=%d\n", 
                       cmd->bank, cmd->address, req->wLength);
                int write_len = MIN((int)req->wLength - offsetof(ps3_cmd_storage_t, write.payload),
                                    MIN((int)sizeof(ps3->bank[0]) - cmd->address, cmd->write.len));
                if (write_len > 0) {
                    memcpy(ps3->bank[cmd->bank] + cmd->address, cmd->write.payload, write_len);
                }
                break;
            default:
                break;
            }
            }
            break;
        case PS3_REQ_FEATURE_REPORT_ID_BT_PAIRING:
            {
            const ps3_bt_pairing_t* pairing = (const ps3_bt_pairing_t*)req->data;
            memcpy(ps3->host_mac_addr, pairing->host_mac_addr, sizeof(ps3->host_mac_addr));
            }
        default:
            break;
        }
        break;
    }
    return true;
}

static bool ps3_handle_get_report(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    const uint8_t report_id = req->wValue & 0xFF;
    const uint8_t report_type = req->wValue >> 8;
    ps3_state_t* ps3 = ps3_state[handle->port];
    ogxm_logd("PS3 Get Report: ID=0x%02X, Type=0x%02X, Length=%d\n", report_id, report_type, req->wLength);

    switch (report_type) {
    case USB_REQ_HID_REPORT_TYPE_INPUT:
        if (report_id == PS3_REPORT_ID_IN) {
            return usbd_send_ctrl_resp(handle, &ps3->report_in, 
                                        PS3_REPORT_IN_SIZE, NULL);
        }
        break;
    case USB_REQ_HID_REPORT_TYPE_FEATURE:
        switch (report_id) {
        case PS3_REQ_FEATURE_REPORT_ID_DS3_INFO:
            return usbd_send_ctrl_resp(handle, &PS3_DEFAULT_DS3_INFO, 
                                       sizeof(PS3_DEFAULT_DS3_INFO), NULL);
        case PS3_REQ_FEATURE_REPORT_ID_STORAGE:
            {
            ogxm_logd("PS3 Get Storage Report: Bank=%d, Address=0x%02X, Length=%d\n", 
                   ps3->bank_select, ps3->bank_address, ps3->bank_req_len);
            int read_len = MIN(sizeof(ps3->bank[0]) - ps3->bank_address, ps3->bank_req_len);
            if (read_len < 0) {
                return false;
            }
            ps3_cmd_storage_resp_t resp = {0};
            resp.report_id = PS3_STORAGE_CMD_READ_RESP_REPORT_ID;
            resp.command = PS3_STORAGE_CMD_READ;
            resp.len = read_len;
            memcpy(resp.payload, ps3->bank[ps3->bank_select] + ps3->bank_address, 
                   read_len);
            return usbd_send_ctrl_resp(handle, &resp, sizeof(resp), NULL);
            }
        case PS3_REQ_FEATURE_REPORT_ID_BT_INFO:
            return usbd_send_ctrl_resp(handle, &ps3->bt_info, 
                                       sizeof(ps3_bt_info_t), NULL);
        case PS3_REQ_FEATURE_REPORT_ID_MOTION_CALIB:
            {
            ps3_motion_calib_t ps3_calib = PS3_DEFAULT_MOTION_CALIB;
            ps3_calib.save_byte = ps3->motion_calib_byte;
            return usbd_send_ctrl_resp(handle, &ps3_calib, sizeof(ps3_calib), NULL);
            }
        case PS3_REQ_FEATURE_REPORT_ID_BT_PAIRING:
            {
            ps3_bt_pairing_t bt_pairing = {0};
            bt_pairing.report_id = PS3_REQ_FEATURE_REPORT_ID_BT_PAIRING;
            memcpy(bt_pairing.host_mac_addr, ps3->host_mac_addr, sizeof(ps3->host_mac_addr));
            return usbd_send_ctrl_resp(handle, &bt_pairing, sizeof(bt_pairing), NULL);
            }
        case PS3_REQ_FEATURE_REPORT_ID_UNK_F7:
            return usbd_send_ctrl_resp(handle, PS3_UNK_FEATURE_F7, 
                                       sizeof(PS3_UNK_FEATURE_F7), NULL);
        case PS3_REQ_FEATURE_REPORT_ID_UNK_F8:
            return usbd_send_ctrl_resp(handle, PS3_UNK_FEATURE_F8, 
                                       sizeof(PS3_UNK_FEATURE_F8), NULL);
        case PS3_REQ_FEATURE_REPORT_ID_UNK_F9:
            return usbd_send_ctrl_resp(handle, PS3_UNK_FEATURE_F9, 
                                       sizeof(PS3_UNK_FEATURE_F9), NULL);
        default:
            break;
        }
        break;
    default:
        break;
    }
    return false;
}

static bool ps3_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    ps3_state_t* ps3 = ps3_state[handle->port];
    switch (req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) {
    case USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE:
        switch (req->bRequest) {
        case USB_REQ_STD_GET_STATUS:
            {
            static const uint16_t status = 0;
            return usbd_send_ctrl_resp(handle, &status,  sizeof(status), NULL);
            }
        default:
            break;
        }
        break;
    case USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE:
        switch (req->bRequest) {
        case USB_REQ_HID_SET_IDLE:
            ps3->idle_rate = (req->wValue >> 8) & 0xFF;
            return true;
        case USB_REQ_HID_GET_IDLE:
            return usbd_send_ctrl_resp(handle, &ps3->idle_rate, 
                                    sizeof(ps3->idle_rate), NULL);
        case USB_REQ_HID_SET_PROTOCOL:
            ps3->protocol = req->wValue;
            return true;
        case USB_REQ_HID_GET_PROTOCOL:
            return usbd_send_ctrl_resp(handle, &ps3->protocol, 
                                       sizeof(ps3->protocol), NULL);
        case USB_REQ_HID_SET_REPORT:
            return ps3_handle_set_report(handle, req);
        case USB_REQ_HID_GET_REPORT:
            return ps3_handle_get_report(handle, req);
        default:
            break;
        }
    default:
        break;
    }
    return false;
}

static bool ps3_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &PS3_DESC_CONFIG);
}

static void ps3_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;

    ps3_state_t* ps3 = ps3_state[handle->port];
    /* BT Info */
    memcpy(&ps3->bt_info, &PS3_DEFAULT_BT_INFO, sizeof(PS3_DEFAULT_BT_INFO));
    for (uint8_t i = 3; i < sizeof(ps3->bt_info.bdaddr); i++) {
        ps3->bt_info.bdaddr[i] = (uint8_t)(get_rand_32() % 0xFF);
    }
    ps3->bt_info.serial = get_rand_32();

    for (uint8_t i = 0; i < sizeof(ps3->host_mac_addr); i++) {
        ps3->host_mac_addr[i] = (uint8_t)(get_rand_32() % 0xFF);
    }

    /* Report In */
    memset(&ps3->report_in, 0, sizeof(ps3->report_in));
    ps3->report_in.report_id       = PS3_REPORT_ID_IN;

    ps3->report_in.joystick_lx     = PS3_JOYSTICK_MID;
    ps3->report_in.joystick_ly     = PS3_JOYSTICK_MID;
    ps3->report_in.joystick_rx     = PS3_JOYSTICK_MID;
    ps3->report_in.joystick_ry     = PS3_JOYSTICK_MID;

    ps3->report_in.plugged         = PS3_PLUGGED;
    ps3->report_in.power_status    = PS3_POWER_FULL;
    ps3->report_in.rumble_status   = PS3_RUMBLE_WIRED_ON;

    ps3->report_in.accel_x  = PS3_SIXAXIS_MID;
    ps3->report_in.accel_y  = PS3_SIXAXIS_MID;
    ps3->report_in.accel_z  = PS3_SIXAXIS_MID;
    ps3->report_in.gyro_z   = PS3_SIXAXIS_MID;

    memcpy(ps3->bank[0], PS3_DEFAULT_STORAGE_BANK_A, sizeof(PS3_DEFAULT_STORAGE_BANK_A));
    memcpy(ps3->bank[1], PS3_DEFAULT_STORAGE_BANK_B, sizeof(PS3_DEFAULT_STORAGE_BANK_B));
}

static void ps3_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    (void)handle;
    (void)epaddr;
}

static usbd_handle_t* ps3_init(const usb_device_driver_cfg_t* cfg) {
    usbd_driver_t driver = {
        .init_cb = ps3_init_cb,
        .deinit_cb = ps3_deinit_cb,
        .get_desc_cb = ps3_get_desc_cb,
        .set_config_cb = ps3_set_config_cb,
        .configured_cb = ps3_configured_cb,
        .ctrl_xfer_cb = ps3_ctrl_xfer_cb,
        .ep_xfer_cb = ps3_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, PS3_EP0_SIZE);
    if (handle != NULL) {
        ps3_state[handle->port] = (ps3_state_t*)cfg->usb.status_buffer;
    }
    return handle;
}

static void ps3_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad) {
    ps3_state_t* ps3 = ps3_state[handle->port];
    if (!ps3->reports_enabled || !(pad->flags & GAMEPAD_FLAG_PAD)) {
        return;
    }
    memset(&ps3->report_in.buttons, 0, sizeof(ps3->report_in.buttons));

    if (pad->buttons & GAMEPAD_BUTTON_UP)       { ps3->report_in.buttons[0] |= PS3_BTN0_DPAD_UP; }
    if (pad->buttons & GAMEPAD_BUTTON_DOWN)     { ps3->report_in.buttons[0] |= PS3_BTN0_DPAD_DOWN; }
    if (pad->buttons & GAMEPAD_BUTTON_LEFT)     { ps3->report_in.buttons[0] |= PS3_BTN0_DPAD_LEFT; }
    if (pad->buttons & GAMEPAD_BUTTON_RIGHT)    { ps3->report_in.buttons[0] |= PS3_BTN0_DPAD_RIGHT; }
    if (pad->buttons & GAMEPAD_BUTTON_START)    { ps3->report_in.buttons[0] |= PS3_BTN0_START; }
    if (pad->buttons & GAMEPAD_BUTTON_BACK)     { ps3->report_in.buttons[0] |= PS3_BTN0_SELECT; }
    if (pad->buttons & GAMEPAD_BUTTON_L3)       { ps3->report_in.buttons[0] |= PS3_BTN0_L3; }
    if (pad->buttons & GAMEPAD_BUTTON_R3)       { ps3->report_in.buttons[0] |= PS3_BTN0_R3; }
    if (pad->buttons & GAMEPAD_BUTTON_LB)       { ps3->report_in.buttons[1] |= PS3_BTN1_L1; }
    if (pad->buttons & GAMEPAD_BUTTON_RB)       { ps3->report_in.buttons[1] |= PS3_BTN1_R1; }
    if (pad->buttons & GAMEPAD_BUTTON_A)        { ps3->report_in.buttons[1] |= PS3_BTN1_CROSS; }
    if (pad->buttons & GAMEPAD_BUTTON_B)        { ps3->report_in.buttons[1] |= PS3_BTN1_CIRCLE; }
    if (pad->buttons & GAMEPAD_BUTTON_X)        { ps3->report_in.buttons[1] |= PS3_BTN1_SQUARE; }
    if (pad->buttons & GAMEPAD_BUTTON_Y)        { ps3->report_in.buttons[1] |= PS3_BTN1_TRIANGLE; }
    if (pad->buttons & GAMEPAD_BUTTON_SYS)      { ps3->report_in.buttons[2] |= PS3_BTN2_SYS; }

    if (pad->trigger_l) { ps3->report_in.buttons[1] |= PS3_BTN1_L2; }
    if (pad->trigger_r) { ps3->report_in.buttons[1] |= PS3_BTN1_R2; }

    ps3->report_in.joystick_lx = range_int16_to_uint8(pad->joystick_lx);
    ps3->report_in.joystick_ly = range_int16_to_uint8(range_invert_int16(pad->joystick_ly));
    ps3->report_in.joystick_rx = range_int16_to_uint8(pad->joystick_rx);
    ps3->report_in.joystick_ry = range_int16_to_uint8(range_invert_int16(pad->joystick_ry));

    ps3->report_in.a_l2 = pad->trigger_l;
    ps3->report_in.a_r2 = pad->trigger_r;

    if (pad->flags & GAMEPAD_FLAG_ANALOG) {
        ps3->report_in.a_up          = pad->analog[GAMEPAD_ANALOG_UP];
        ps3->report_in.a_right       = pad->analog[GAMEPAD_ANALOG_RIGHT];
        ps3->report_in.a_down        = pad->analog[GAMEPAD_ANALOG_DOWN];
        ps3->report_in.a_left        = pad->analog[GAMEPAD_ANALOG_LEFT];
        ps3->report_in.a_l1          = pad->analog[GAMEPAD_ANALOG_LB];
        ps3->report_in.a_r1          = pad->analog[GAMEPAD_ANALOG_RB];
        ps3->report_in.a_triangle    = pad->analog[GAMEPAD_ANALOG_Y];
        ps3->report_in.a_circle      = pad->analog[GAMEPAD_ANALOG_B];
        ps3->report_in.a_cross       = pad->analog[GAMEPAD_ANALOG_A];
        ps3->report_in.a_square      = pad->analog[GAMEPAD_ANALOG_X];
    } else {
        ps3->report_in.a_up          = (pad->dpad & GAMEPAD_BUTTON_UP)    ? 0xFF : 0x00;
        ps3->report_in.a_right       = (pad->dpad & GAMEPAD_BUTTON_RIGHT) ? 0xFF : 0x00;
        ps3->report_in.a_down        = (pad->dpad & GAMEPAD_BUTTON_DOWN)  ? 0xFF : 0x00;
        ps3->report_in.a_left        = (pad->dpad & GAMEPAD_BUTTON_LEFT)  ? 0xFF : 0x00;
        ps3->report_in.a_l1          = (pad->buttons & GAMEPAD_BUTTON_LB)  ? 0xFF : 0x00;
        ps3->report_in.a_r1          = (pad->buttons & GAMEPAD_BUTTON_RB)  ? 0xFF : 0x00;
        ps3->report_in.a_triangle    = (pad->buttons & GAMEPAD_BUTTON_Y)   ? 0xFF : 0x00;
        ps3->report_in.a_circle      = (pad->buttons & GAMEPAD_BUTTON_B)   ? 0xFF : 0x00;
        ps3->report_in.a_cross       = (pad->buttons & GAMEPAD_BUTTON_A)   ? 0xFF : 0x00;
        ps3->report_in.a_square      = (pad->buttons & GAMEPAD_BUTTON_X)   ? 0xFF : 0x00;
    }
    if (usbd_ep_ready(handle, PS3_EPADDR_IN)) {
        usbd_ep_write(handle, PS3_EPADDR_IN, &ps3->report_in, PS3_REPORT_IN_SIZE);
    }
}

static void ps3_task(usbd_handle_t* handle) {
    ps3_state_t* ps3 = ps3_state[handle->port];
    if (!ps3->reports_enabled || !usbd_ep_ready(handle, PS3_EPADDR_IN)) {
        return;
    }
    /* PS3 expects constant reports every frame */
    usbd_ep_write(handle, PS3_EPADDR_IN, &ps3->report_in, PS3_REPORT_IN_SIZE);
}

const usb_device_driver_t USBD_DRIVER_PS3 = {
    .name = "PS3",
    .init = ps3_init,
    .task = ps3_task,
    .set_pad = ps3_set_pad,
    .set_audio = NULL,
};