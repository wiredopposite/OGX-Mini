#include <stdio.h>
#include <string.h>
#include "g72x.h"
#include "xsm3.h"
#include "usb/device/device.h"
#include "usb/descriptors/xinput.h"
#include "usb/device/device_private.h"
#include "assert_compat.h"

#define G726_COMPRESSION_RATIO      8U
#define PCM_SAMPES_PER_G726_BYTE    4U

typedef enum {
    XINPUT_AUTH_REQ_1 = 0x81,
    XINPUT_AUTH_REQ_2 = 0x82,
    XINPUT_AUTH_REQ_3 = 0x87,
    XINPUT_AUTH_REQ_4 = 0x83,
    XINPUT_AUTH_REQ_5 = 0x86,
} xinput_auth_req_t;

typedef enum {
    XINPUT_AUDIO_INIT_0 = 0,
    XINPUT_AUDIO_INIT_1,
    XINPUT_AUDIO_INIT_2,
    XINPUT_AUDIO_INIT_3,
    XINPUT_AUDIO_INIT_4,
    XINPUT_AUDIO_INIT_5,
    XINPUT_AUDIO_INIT_6,
    XINPUT_AUDIO_INIT_7,
    XINPUT_AUDIO_INIT_DONE
} xinput_audio_state_t;

typedef struct {
    /* Gamepad */
    xinput_report_in_t      report_in;
    xinput_report_out_t     report_out;
    gamepad_rumble_t        gp_rumble;
    gamepad_pad_t           gp_pad;
    /* Audio */
    g726_state              g726_ctx_encode;
    g726_state              g726_ctx_decode;
    bool                    audio_en;
    xinput_audio_state_t    audio_state;
    uint8_t                 g726_in[XINPUT_EPSIZE_AUDIO_IN];
    gamepad_pcm_out_t       pcm_out;
    gamepad_pcm_in_t        pcm_in;
} xinput_state_t;
_STATIC_ASSERT(sizeof(xinput_state_t) <= USBD_STATUS_BUF_SIZE, "XINPUT state size exceeds buffer size");

static const uint8_t XINPUT_VENDOR_BLOB[] = {
	0x28, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x58, 0x55, 0x53, 0x42, 0x31, 0x30, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static xinput_state_t* xinput_state[USBD_DEVICES_MAX] = { NULL };

static const xinput_audio_ctrl_t AUDIO_INIT_SEQ[XINPUT_AUDIO_INIT_DONE] = {
	{ .report_id = 0x05, .length = 3, .data = { 0x00 } },
	{ .report_id = 0x00, .length = 4, .data = { 0x01, 0x00 } },
	{ .report_id = 0x01, .length = 3, .data = { 0x00 } },
	{ .report_id = 0x02, .length = 3, .data = { 0x0B } },
	{ .report_id = 0x03, .length = 5, .data = { 0xFF, 0x00, 0x00 } },
	{ .report_id = 0x04, .length = 3, .data = { 0xFF } },
	{ .report_id = 0x05, .length = 3, .data = { 0x00 } },
};

/* Encodes 4 pcm samples at a time */
static inline uint8_t headset_encode_byte(g726_state* ctx, const int16_t* pcm_in) {
    return  ((g726_16_encoder(pcm_in[0], AUDIO_ENCODING_LINEAR, ctx) & 0x03) << 6) |
            ((g726_16_encoder(pcm_in[1], AUDIO_ENCODING_LINEAR, ctx) & 0x03) << 4) |
            ((g726_16_encoder(pcm_in[2], AUDIO_ENCODING_LINEAR, ctx) & 0x03) << 2) |
            (g726_16_encoder(pcm_in[3], AUDIO_ENCODING_LINEAR, ctx) & 0x03);
}

/* Decodes 4 pcm samples at a time */
static void headset_decode_byte(g726_state* ctx, const uint8_t g726_in, int16_t* pcm_out) {
    int code = (g726_in >> 6) & 0x03;
    pcm_out[0] = g726_16_decoder(code, AUDIO_ENCODING_LINEAR, ctx);

    code = (g726_in >> 4) & 0x03;
    pcm_out[1] = g726_16_decoder(code, AUDIO_ENCODING_LINEAR, ctx);

    code = (g726_in >> 2) & 0x03;
    pcm_out[2] = g726_16_decoder(code, AUDIO_ENCODING_LINEAR, ctx);

    code = g726_in & 0x03;
    pcm_out[3] = g726_16_decoder(code, AUDIO_ENCODING_LINEAR, ctx);
}

static void xinput_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void xinput_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
}

static bool xinput_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &XINPUT_DESC_DEVICE,
                                   XINPUT_DESC_DEVICE.bLength, NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &XINPUT_DESC_CONFIG,
                                   XINPUT_DESC_CONFIG.config.wTotalLength, NULL);
    case USB_DTYPE_STRING:
        {
        const uint8_t idx = USB_DESC_INDEX(req->wValue);
        if (idx < ARRAY_SIZE(XINPUT_DESC_STRING)) {
            return usbd_send_ctrl_resp(handle, XINPUT_DESC_STRING[idx],
                                        XINPUT_DESC_STRING[idx]->bLength, NULL);
        }
        }
        break;
    default:
        break;
    }
    return false;
}

static bool xinput_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &XINPUT_DESC_CONFIG);
}

static void xinput_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    xinput_state_t* xinput = xinput_state[handle->port];
    memset(&xinput->report_in, 0, sizeof(xinput->report_in));
    xinput->report_in.length = sizeof(xinput->report_in);
    // uint8_t start[] = {0x01, 0x03, 0x0E};
    // usbd_ep_write(handle, XINPUT_EPADDR_GP_IN, &start, sizeof(start));
}

static bool xinput_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    xinput_state_t* state = xinput_state[handle->port];
    switch (req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) {
    case USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE:
        switch (req->bRequest) {
        case USB_REQ_STD_SET_INTERFACE:
            return true;
        case USB_REQ_STD_GET_STATUS:
            {
            static const uint8_t status[2] = { 0x02, 0x00 };
            return usbd_send_ctrl_resp(handle, status, sizeof(status), NULL);
            }
        default:
            break;
        }
        break;
    case USB_REQ_TYPE_VENDOR | USB_REQ_RECIP_DEVICE:
        switch (req->bRequest) {
        // case XINPUT_AUTH_REQ_1:
        // // xsm3_set_vid_pid(XINPUT_VID, XINPUT_PID);
        //     xsm3_initialise_state();
        //     xsm3_set_identification_data(xsm3_id_data_ms_controller);
        //     return usbd_send_ctrl_resp(handle, xsm3_id_data_ms_controller,
        //                                 sizeof(xsm3_id_data_ms_controller), NULL);
        // case XINPUT_AUTH_REQ_2:
        //     xsm3_do_challenge_init(req->data);
        //     return true;
        // case XINPUT_AUTH_REQ_3:
        //     xsm3_do_challenge_verify(req->data);
        //     return true;
        // case XINPUT_AUTH_REQ_4:
        //     return usbd_send_ctrl_resp(handle, xsm3_challenge_response,
        //                                 sizeof(xsm3_challenge_response), NULL);
        // case XINPUT_AUTH_REQ_5:
        //     uint16_t state = 2; // 1 = in-progress, 2 = complete
        //     return usbd_send_ctrl_resp(handle, &state, sizeof(state), NULL);
        case 144:
            if (req->wIndex == 4) {
                return usbd_send_ctrl_resp(handle, XINPUT_VENDOR_BLOB, 
                                           sizeof(XINPUT_VENDOR_BLOB), NULL);
            } 
            break;
        case 1:
            if (req->wValue == 0 && (req->wIndex == 0)) {
                const uint8_t status[4] = {0x03, 0xF8, 0x86, 0x28};
                return usbd_send_ctrl_resp(handle, status, sizeof(status), NULL);
            } 
            break;
        default:
            break;
        }
        break;
    case USB_REQ_TYPE_VENDOR | USB_REQ_RECIP_INTERFACE:
        switch (req->bRequest) {
        case 1:
            if (req->wValue == 0 && req->wIndex == 0) {
                // STALL
            } else if (req->wValue == 0x0100 && (req->wIndex == 0)) {
                const uint8_t status[3] = {0x01, 0x03, 0x0E};
                return usbd_send_ctrl_resp(handle, status, sizeof(status), NULL);
            } 
            break;
        default:
            break;
        }
        break;
    case USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE:
        switch (req->bRequest) {
        case USB_REQ_HID_SET_REPORT:
            // {
            // const xinput_report_out_t* report_out = (const xinput_report_out_t*)req->data;
            // if ((req->wLength >= sizeof(xinput_report_out_t)) &&
            //     (state->rumble_cb != NULL) &&
            //     (report_out->report_id == XINPUT_REPORT_ID_OUT_RUMBLE)) {
            //     state->gp_rumble.l = report_out->rumble_l;
            //     state->gp_rumble.r = report_out->rumble_r;
            //     state->rumble_cb(handle, &state->gp_rumble);
            // }
            // }
            return true;
        case USB_REQ_HID_GET_REPORT:
            return usbd_send_ctrl_resp(handle, &state->report_in,
                                       sizeof(xinput_report_in_t), NULL);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return false;
}

static void xinput_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    xinput_state_t* xinput = xinput_state[handle->port];
    switch (epaddr) {
    case XINPUT_EPADDR_GP_OUT:
        {
        int32_t len = usbd_ep_read(handle, XINPUT_EPADDR_GP_OUT,
                                   &xinput->report_out, sizeof(xinput->report_out));
        if ((len >= sizeof(xinput_report_out_t)) &&
            (xinput->report_out.report_id == XINPUT_REPORT_ID_OUT_RUMBLE)) {
            xinput->gp_rumble.l = xinput->report_out.rumble_l;
            xinput->gp_rumble.r = xinput->report_out.rumble_r;
            usb_device_rumble_cb(handle, &xinput->gp_rumble);
        }
        }
        break;
    case XINPUT_EPADDR_GP_IN:
        /* Report sent */
        break;
    case XINPUT_EPADDR_AUDIO_OUT:
        break;
    case XINPUT_EPADDR_AUDIO_IN:
        break;
    case XINPUT_EPADDR_AUDIO_CTRL_OUT:
        if (xinput->audio_en && (xinput->audio_state == XINPUT_AUDIO_INIT_0)) {
            usbd_ep_write(handle, XINPUT_EPADDR_AUDIO_CTRL_IN,
                          AUDIO_INIT_SEQ[xinput->audio_state].data,
                          AUDIO_INIT_SEQ[xinput->audio_state].length);
            xinput->audio_state++;
        }
        usbd_ep_flush(handle, XINPUT_EPADDR_AUDIO_CTRL_OUT);
        break;
    case XINPUT_EPADDR_AUDIO_CTRL_IN:
        if (xinput->audio_en && (xinput->audio_state < XINPUT_AUDIO_INIT_DONE)) {
            usbd_ep_write(handle, XINPUT_EPADDR_AUDIO_CTRL_IN,
                          AUDIO_INIT_SEQ[xinput->audio_state].data,
                          AUDIO_INIT_SEQ[xinput->audio_state].length);
            xinput->audio_state++;
        } else if (xinput->audio_en) {
            xinput->audio_state = XINPUT_AUDIO_INIT_0;
        }
        break;
    default:
        break;
    }
}

static usbd_handle_t* xinput_init(const usb_device_driver_cfg_t* cfg) {
    if ((cfg == NULL) || (cfg->usb.status_buffer == NULL)) {
        return NULL;
    }
    usbd_driver_t driver = {
        .init_cb = xinput_init_cb,
        .deinit_cb = xinput_deinit_cb,
        .get_desc_cb = xinput_get_desc_cb,
        .set_config_cb = xinput_set_config_cb,
        .configured_cb = xinput_configured_cb,
        .ctrl_xfer_cb = xinput_ctrl_xfer_cb,
        .ep_xfer_cb = xinput_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, XINPUT_EPSIZE_CTRL);
    if (handle != NULL) {
        xinput_state[handle->port] = (xinput_state_t*)cfg->usb.status_buffer;
        memset(xinput_state[handle->port], 0, sizeof(xinput_state_t));
        xinput_state[handle->port]->audio_en  = (cfg->usb.addons & USBD_ADDON_HEADSET);
        if (xinput_state[handle->port]->audio_en) {
            g726_init_state(&xinput_state[handle->port]->g726_ctx_encode);
            g726_init_state(&xinput_state[handle->port]->g726_ctx_decode);
        }
    }
    return handle;
}

// static void xinput_send_audio(usbd_handle_t* handle, const gamepad_pcm_in_t* pcm) {
//     if (xinput_state[handle->port] == NULL) {
//         return;
//     }
//     xinput_state_t* xinput = xinput_state[handle->port];
//     if (xinput->audio_en && (xinput->audio_state == XINPUT_AUDIO_INIT_DONE)) {
//         const int16_t* pcm_start = (const int16_t*)pcm->data;
//         const int16_t* pcm_end = pcm_start + pcm->samples;
//         uint16_t idx = 0;

//         while (((pcm_start + PCM_SAMPES_PER_G726_BYTE) <= pcm_end) && (idx < sizeof(xinput->g726_in))) {
//             xinput->g726_in[idx++] = headset_encode_byte(&xinput->g726_ctx_encode, pcm_start);
//             pcm_start += PCM_SAMPES_PER_G726_BYTE;
//         }
//         if (idx < sizeof(xinput->g726_in)) {
//             const int16_t silence[PCM_SAMPES_PER_G726_BYTE] = {0};
//             while (idx < sizeof(xinput->g726_in)) {
//                 xinput->g726_in[idx++] = headset_encode_byte(&xinput->g726_ctx_encode, silence);
//             }
//         }
//         usbd_ep_write(handle, XINPUT_EPADDR_AUDIO_IN, xinput->g726_in, idx);
//     }
// }

// static void xinput_task(usbd_handle_t* handle, gamepad_handle_t* gp_handle) {
//     xinput_state_t* xinput = xinput_state[handle->port];
//     if (usbd_ep_ready(handle, XINPUT_EPADDR_GP_IN)) {
//         gamepad_pad_t* gp = &xinput->gp_pad;
//         uint32_t gp_flags = gamepad_get_pad(gp_handle, gp);

//         if (gp_flags & GAMEPAD_FLAG_IN_PAD) {
//             xinput->report_in.buttons = 0;

//             if (gp->dpad & GAMEPAD_D_UP)         { xinput->report_in.buttons |= XINPUT_BUTTON_UP; }
//             if (gp->dpad & GAMEPAD_D_DOWN)       { xinput->report_in.buttons |= XINPUT_BUTTON_DOWN; }
//             if (gp->dpad & GAMEPAD_D_LEFT)       { xinput->report_in.buttons |= XINPUT_BUTTON_LEFT; }
//             if (gp->dpad & GAMEPAD_D_RIGHT)      { xinput->report_in.buttons |= XINPUT_BUTTON_RIGHT; }
//             if (gp->buttons & GAMEPAD_BTN_START) { xinput->report_in.buttons |= XINPUT_BUTTON_START; }
//             if (gp->buttons & GAMEPAD_BTN_BACK)  { xinput->report_in.buttons |= XINPUT_BUTTON_BACK; }
//             if (gp->buttons & GAMEPAD_BTN_L3)    { xinput->report_in.buttons |= XINPUT_BUTTON_L3; }
//             if (gp->buttons & GAMEPAD_BTN_R3)    { xinput->report_in.buttons |= XINPUT_BUTTON_R3; }
//             if (gp->buttons & GAMEPAD_BTN_LB)    { xinput->report_in.buttons |= XINPUT_BUTTON_LB; }
//             if (gp->buttons & GAMEPAD_BTN_RB)    { xinput->report_in.buttons |= XINPUT_BUTTON_RB; }
//             if (gp->buttons & GAMEPAD_BTN_SYS)   { xinput->report_in.buttons |= XINPUT_BUTTON_HOME; }
//             if (gp->buttons & GAMEPAD_BTN_A)     { xinput->report_in.buttons |= XINPUT_BUTTON_A; }
//             if (gp->buttons & GAMEPAD_BTN_B)     { xinput->report_in.buttons |= XINPUT_BUTTON_B; }
//             if (gp->buttons & GAMEPAD_BTN_X)     { xinput->report_in.buttons |= XINPUT_BUTTON_X; }
//             if (gp->buttons & GAMEPAD_BTN_Y)     { xinput->report_in.buttons |= XINPUT_BUTTON_Y; }

//             xinput->report_in.trigger_l = gp->trigger_l;
//             xinput->report_in.trigger_r = gp->trigger_r;
//             xinput->report_in.joystick_lx = gp->joystick_lx;
//             xinput->report_in.joystick_ly = gp->joystick_ly;
//             xinput->report_in.joystick_rx = gp->joystick_rx;
//             xinput->report_in.joystick_ry = gp->joystick_ry;

//             usbd_ep_write(handle, XINPUT_EPADDR_GP_IN, &xinput->report_in, sizeof(xinput->report_in));
//         }
//     }
//     if (!xinput->audio_en || (xinput->audio_state != XINPUT_AUDIO_INIT_DONE)) {
//         return;
//     }
//     // gamepad_pcm_in_t* pcm = &xinput->pcm_in;
//     // gp_flags = gamepad_get_pcm_in(gp_handle, pcm);
//     // if ((gp_flags & GAMEPAD_FLAG_PCM_IN) && (pcm->samples > 0)) {
//     //     xinput_send_audio(handle, pcm);
//     // } else if (xinput->audio_en && (xinput->audio_state == XINPUT_AUDIO_INIT_DONE)) {
//     //     /* Send silence if no audio data is available */
//     //     const int16_t silence[PCM_SAMPES_PER_G726_BYTE] = {0};
//     //     for (uint8_t i = 0; i < sizeof(xinput->g726_in); i++) {
//     //         xinput->g726_in[i] = headset_encode_byte(&xinput->g726_ctx_encode, silence);
//     //     }
//     //     usbd_ep_write(handle, XINPUT_EPADDR_AUDIO_IN, xinput->g726_in, sizeof(xinput->g726_in));
//     // }
// }

static void xinput_send_audio(usbd_handle_t* handle, const gamepad_pcm_in_t* pcm) {
    if (xinput_state[handle->port] == NULL) {
        return;
    }
    xinput_state_t* xinput = xinput_state[handle->port];
    if (xinput->audio_en && (xinput->audio_state == XINPUT_AUDIO_INIT_DONE)) {
        const int16_t* pcm_start = (const int16_t*)pcm->data;
        const int16_t* pcm_end = pcm_start + pcm->samples;
        uint16_t idx = 0;

        while (((pcm_start + PCM_SAMPES_PER_G726_BYTE) <= pcm_end) && (idx < sizeof(xinput->g726_in))) {
            xinput->g726_in[idx++] = headset_encode_byte(&xinput->g726_ctx_encode, pcm_start);
            pcm_start += PCM_SAMPES_PER_G726_BYTE;
        }
        if (idx < sizeof(xinput->g726_in)) {
            const int16_t silence[PCM_SAMPES_PER_G726_BYTE] = {0};
            while (idx < sizeof(xinput->g726_in)) {
                xinput->g726_in[idx++] = headset_encode_byte(&xinput->g726_ctx_encode, silence);
            }
        }
        usbd_ep_write(handle, XINPUT_EPADDR_AUDIO_IN, xinput->g726_in, idx);
    }
}

static void xinput_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad, uint32_t flags) {
    xinput_state_t* xinput = xinput_state[handle->port];
    if (!usbd_ep_ready(handle, XINPUT_EPADDR_GP_IN) || !(flags & GAMEPAD_FLAG_IN_PAD)) {
        return;
    }
    xinput->report_in.buttons = 0;

    if (pad->dpad & GAMEPAD_D_UP)         { xinput->report_in.buttons |= XINPUT_BUTTON_UP; }
    if (pad->dpad & GAMEPAD_D_DOWN)       { xinput->report_in.buttons |= XINPUT_BUTTON_DOWN; }
    if (pad->dpad & GAMEPAD_D_LEFT)       { xinput->report_in.buttons |= XINPUT_BUTTON_LEFT; }
    if (pad->dpad & GAMEPAD_D_RIGHT)      { xinput->report_in.buttons |= XINPUT_BUTTON_RIGHT; }
    if (pad->buttons & GAMEPAD_BTN_START) { xinput->report_in.buttons |= XINPUT_BUTTON_START; }
    if (pad->buttons & GAMEPAD_BTN_BACK)  { xinput->report_in.buttons |= XINPUT_BUTTON_BACK; }
    if (pad->buttons & GAMEPAD_BTN_L3)    { xinput->report_in.buttons |= XINPUT_BUTTON_L3; }
    if (pad->buttons & GAMEPAD_BTN_R3)    { xinput->report_in.buttons |= XINPUT_BUTTON_R3; }
    if (pad->buttons & GAMEPAD_BTN_LB)    { xinput->report_in.buttons |= XINPUT_BUTTON_LB; }
    if (pad->buttons & GAMEPAD_BTN_RB)    { xinput->report_in.buttons |= XINPUT_BUTTON_RB; }
    if (pad->buttons & GAMEPAD_BTN_SYS)   { xinput->report_in.buttons |= XINPUT_BUTTON_HOME; }
    if (pad->buttons & GAMEPAD_BTN_A)     { xinput->report_in.buttons |= XINPUT_BUTTON_A; }
    if (pad->buttons & GAMEPAD_BTN_B)     { xinput->report_in.buttons |= XINPUT_BUTTON_B; }
    if (pad->buttons & GAMEPAD_BTN_X)     { xinput->report_in.buttons |= XINPUT_BUTTON_X; }
    if (pad->buttons & GAMEPAD_BTN_Y)     { xinput->report_in.buttons |= XINPUT_BUTTON_Y; }

    xinput->report_in.trigger_l = pad->trigger_l;
    xinput->report_in.trigger_r = pad->trigger_r;
    xinput->report_in.joystick_lx = pad->joystick_lx;
    xinput->report_in.joystick_ly = pad->joystick_ly;
    xinput->report_in.joystick_rx = pad->joystick_rx;
    xinput->report_in.joystick_ry = pad->joystick_ry;

    usbd_ep_write(handle, XINPUT_EPADDR_GP_IN, &xinput->report_in, sizeof(xinput->report_in));
}

const usb_device_driver_t USBD_DRIVER_XINPUT = {
    .name = "XInput",
    .init = xinput_init,
    .task = NULL,
    .set_audio = NULL,
    .set_pad = xinput_set_pad,
};