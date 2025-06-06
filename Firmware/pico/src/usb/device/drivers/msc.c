#include "board_config.h"
#include "usbd/usbd.h"
#include "usb/device/device_private.h"
#if SD_CARD_ENABLED

#include <string.h>
#include <stdio.h>
#include "common/usb_util.h"
#include "common/class/msc_def.h"
#include "gamepad/range.h"
#include "sdcard/sdcard.h"
#include "usb/descriptors/msc.h"
#include "usb/device/device.h"
#include "assert_compat.h"

#define MSC_BLOCK_SIZE 512U

#define TO_UINT32(p) ((uint32_t)((((uint8_t*)(p))[0] << 24) | (((uint8_t*)(p))[1] << 16) | (((uint8_t*)(p))[2] << 8) | ((uint8_t*)(p))[3]))
#define TO_UINT16(p) ((uint16_t)((((uint8_t*)(p))[0] << 8) | ((uint8_t*)(p))[1]))
#define SWAP16(x)   ((uint16_t)(((x) >> 8) | ((x) << 8)))
#define SWAP32(x)   ((uint32_t)(((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24)))

typedef struct {
    bool ready;
    uint32_t block_count;
    usb_msc_csw_t csw;

    bool out_pending;
    uint32_t out_tag;
    uint32_t out_lba;
    uint16_t out_count;
    uint16_t out_idx;
    uint16_t out_current_count;

    uint8_t block_out[MSC_BLOCK_SIZE] __attribute__((aligned(4)));
    uint8_t block_in[MSC_BLOCK_SIZE] __attribute__((aligned(4)));
    uint8_t ep_out[MSC_EPSIZE_OUT] __attribute__((aligned(4)));
} msc_state_t;
_STATIC_ASSERT(sizeof(msc_state_t) <= USBD_STATUS_BUF_SIZE, "XBOXOG GP state size exceeds buffer size");

typedef struct __attribute__((packed)) {
    uint8_t  opcode;         // 0x28 for READ(10), 0x2A for WRITE(10)
    uint8_t  flags;
    uint32_t lba;            // Logical Block Address (big endian)
    uint8_t  reserved;
    uint16_t transfer_len;   // Number of blocks (big endian)
    uint8_t  control;
} usb_msc_scsi_rw10_cb_t;

typedef struct __attribute__((packed)) {
    uint8_t peripheral;      // 0x00: Direct-access block device
    uint8_t removable;       // 0x80: Removable
    uint8_t version;         // 0x00: No standard claimed
    uint8_t response_format; // 0x01: Response data format
    uint8_t additional_len;  // n-4 (number of remaining bytes)
    uint8_t sccs;            // 0x00
    uint8_t bque_encserv;    // 0x00
    uint8_t cmdque;          // 0x00
    // char dev_info[]; // Device information string, e.g. "OGXmini SD Storage 1.00"
    char vendor_id[8];       // e.g. "OGXmini "
    char product_id[16];     // e.g. "SD Storage      "
    char product_rev[4];     // e.g. "1.00"
} usb_msc_scsi_inquiry_data_t;

static const usb_msc_scsi_inquiry_data_t MSC_SCSI_INQUIRY_DATA = {
    .peripheral = 0x00, // Direct-access block device
    .removable = 0x80,  // Removable
    .version = 0x00,    // No standard claimed
    .response_format = 0x01, // Response data format
    .additional_len = sizeof(usb_msc_scsi_inquiry_data_t) - 4,
    .sccs = 0x00,       // No SCCS support
    .bque_encserv = 0x00, // No BQUE or ENC_SERV support
    .cmdque = 0x00,     // No command queuing support
    .vendor_id = "OGXmini ",
    .product_id = "SD Storage      ",
    .product_rev = "1.00"
};

static msc_state_t* msc_state[USBD_DEVICES_MAX] = { NULL };

static void msc_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void msc_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool msc_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &MSC_DESC_DEVICE,
                                   sizeof(MSC_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &MSC_DESC_CONFIG,
                                   sizeof(MSC_DESC_CONFIG), NULL);
    case USB_DTYPE_STRING:
        {
        const uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (idx < ARRAY_SIZE(MSC_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, MSC_DESC_STRING[idx],
                                       MSC_DESC_STRING[idx]->bLength, NULL);
        } else if (idx == MSC_DESC_DEVICE.iSerialNumber) {
            const usb_desc_string_t* serial = usbd_get_desc_string_serial(handle);
            return usbd_send_ctrl_resp(handle, serial, serial->bLength, NULL);
        }
        }
    default:
        break;
    }
    return false;
}

static bool msc_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    msc_state_t* msc = msc_state[handle->port];
    switch (req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) {
    case (USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE):
        switch (req->bRequest) {
        case USB_REQ_MSC_GET_MAX_LUN:
            {
            static const uint8_t max_lun = 0; // Only one LUN supported
            return usbd_send_ctrl_resp(handle, &max_lun, sizeof(max_lun), NULL);
            }
        case USB_REQ_MSC_RESET:

            return true;
        default:
            break;
        }
        break;
    case (USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE):
        switch (req->bRequest) {
        case USB_REQ_STD_SET_INTERFACE:
            return true;
        case USB_REQ_STD_GET_STATUS:
            {
            static const uint16_t status = 0;
            return usbd_send_ctrl_resp(handle, &status, sizeof(status), NULL);
            }
        default:
            break;
        }
        break;
    default:
        break;
    }
    return false;
}

static bool msc_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &MSC_DESC_CONFIG);
}

static void msc_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    msc_state_t* msc = msc_state[handle->port];
    memset(msc, 0, sizeof(msc_state_t));
}

static bool msc_write_bulk_blocking(usbd_handle_t* handle, const uint8_t* data, uint16_t len) {
    msc_state_t* msc = msc_state[handle->port];
    uint32_t remaining = len;
    while (remaining > 0) {
        uint16_t to_write = MIN(remaining, MSC_EPSIZE_IN);
        int32_t written = usbd_ep_write(handle, MSC_EPADDR_IN, data + (len - remaining), to_write);
        if (written < 0) {
            printf("Error writing to MSC IN endpoint\n");
            return false;
        }
        while (!usbd_ep_ready(handle, MSC_EPADDR_IN)) {
            usbd_task();
        }
        remaining -= written;
    }
    return true;
}

static void handle_cbw_command(usbd_handle_t* handle, const usb_msc_cbw_t* cbw) {
    msc_state_t* msc = msc_state[handle->port];
    bool error = false;
    switch (cbw->command[0]) {
    case USB_MSC_SCSI_CMD_INQUIRY:
        error = !msc_write_bulk_blocking(handle, (const uint8_t*)&MSC_SCSI_INQUIRY_DATA, 
                                    sizeof(MSC_SCSI_INQUIRY_DATA));
        break;
    case USB_MSC_SCSI_CMD_TEST_UNIT_READY:
        break;
    case USB_MSC_SCSI_CMD_READ_CAPACITY_10:
        {
        uint32_t last_block = sd_msc_get_block_count() - 1;
        uint8_t cap[8] __attribute__((aligned(4))) = {
            (last_block >> 24) & 0xFF, (last_block >> 16) & 0xFF,
            (last_block >> 8) & 0xFF, last_block & 0xFF,
            (MSC_BLOCK_SIZE >> 24) & 0xFF, (MSC_BLOCK_SIZE >> 16) & 0xFF,
            (MSC_BLOCK_SIZE >> 8) & 0xFF, MSC_BLOCK_SIZE & 0xFF
        };
        error = !msc_write_bulk_blocking(handle, cap, sizeof(cap));
        }
        break;
    case USB_MSC_SCSI_CMD_READ_10:
        {
        uint32_t lba = TO_UINT32(&cbw->command[2]);
        uint16_t count = TO_UINT16(&cbw->command[7]);
        for (uint16_t i = 0; i < count; i++) {
            sd_msc_read_blocks(msc->block_in, lba + i, 1);
            if (!msc_write_bulk_blocking(handle, msc->block_in, MSC_BLOCK_SIZE)) {
                error = true;
                break;
            }
        }
        }
        break;
    case USB_MSC_SCSI_CMD_WRITE_10:
        {
        msc->out_lba = TO_UINT32(&cbw->command[2]);
        msc->out_count = TO_UINT16(&cbw->command[7]);
        msc->out_pending = true;
        msc->out_idx = 0;
        msc->out_current_count = 0;
        msc->out_tag = cbw->tag;
        }
        return;
    case USB_MSC_SCSI_CMD_REQUEST_SENSE:
        {
        static const uint8_t sense[18] = {
            0x70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };
        error = !msc_write_bulk_blocking(handle, sense, sizeof(sense));
        }
        break;
    default:
        break;
    }
    msc->csw.signature = USB_MSC_CSW_SIGNATURE;
    msc->csw.tag = cbw->tag;
    msc->csw.data_residue = 0; // No data residue for now
    msc->csw.status = error ? USB_MSC_CSW_STATUS_FAILED : USB_MSC_CSW_STATUS_PASSED; // Assume success for now
    usbd_ep_write(handle, MSC_EPADDR_IN, (const uint8_t*)&msc->csw, sizeof(msc->csw));
}

static void msc_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    msc_state_t* msc = msc_state[handle->port];
    if (epaddr == MSC_EPADDR_OUT) {
        int32_t len = usbd_ep_read(handle, MSC_EPADDR_OUT, 
                                   msc->ep_out, MSC_EPSIZE_OUT);
        if (len < 0) {
            printf("Error reading MSC OUT endpoint\n");
            return;
        }
        if ((len == sizeof(usb_msc_cbw_t)) && 
            (((usb_msc_cbw_t*)msc->ep_out)->signature == USB_MSC_CBW_SIGNATURE)) {
            usb_msc_cbw_t* cbw = (usb_msc_cbw_t*)msc->ep_out;
            handle_cbw_command(handle, cbw);
        } else if (msc->out_pending) {
            memcpy(msc->block_out + msc->out_idx, msc->ep_out, len);
            msc->out_idx += len;

            if (msc->out_idx >= MSC_BLOCK_SIZE) {
                // We have a full block, process it
                sd_msc_write_blocks(msc->block_out, msc->out_lba + msc->out_current_count, 1);
                msc->out_current_count++;
                msc->out_idx -= MSC_BLOCK_SIZE;

                if (msc->out_current_count >= msc->out_count) {
                    // All blocks written, reset state
                    msc->out_pending = false;
                    msc->csw.signature = USB_MSC_CSW_SIGNATURE;
                    msc->csw.tag = msc->out_tag; 
                    msc->csw.data_residue = msc->out_idx;
                    msc->csw.status = USB_MSC_CSW_STATUS_PASSED;
                    usbd_ep_write(handle, MSC_EPADDR_IN, (const uint8_t*)&msc->csw, sizeof(msc->csw));
                } else if (msc->out_idx > 0) {
                    // If there are leftover bytes, shift them to the start
                    memmove(msc->block_out, msc->block_out + MSC_BLOCK_SIZE, msc->out_idx);
                }
            }
        }
    }
}

static usbd_handle_t* msc_init(const usb_device_driver_cfg_t* cfg) {
    if ((cfg == NULL) || (cfg->usb.status_buffer == NULL)) {
        return NULL;
    }
    usbd_driver_t driver = {
        .init_cb = msc_init_cb,
        .deinit_cb = msc_deinit_cb,
        .get_desc_cb = msc_get_desc_cb,
        .set_config_cb = msc_set_config_cb,
        .configured_cb = msc_configured_cb,
        .ctrl_xfer_cb = msc_ctrl_xfer_cb,
        .ep_xfer_cb = msc_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, MSC_EPSIZE_CTRL);
    if (handle != NULL) {
        msc_state[handle->port] = (msc_state_t*)cfg->usb.status_buffer;
    }
    return handle;
}

#else // SD_CARD_ENABLED

static usbd_handle_t* msc_init(const usb_device_driver_cfg_t* cfg) {
    (void)cfg;
    return NULL;
}

#endif // SD_CARD_ENABLED

const usb_device_driver_t USBD_DRIVER_MSC = {
    .name = "MSC",
    .init = msc_init,
    .task = NULL,
    .set_audio = NULL,
    .set_pad = NULL
};