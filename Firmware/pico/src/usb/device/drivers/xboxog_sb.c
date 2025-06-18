#include <string.h>
#include <stdlib.h>
#include <pico/time.h>
#include "common/class/hid_def.h"
#include "usbd/usbd.h"
#include "gamepad/range.h"
#include "usb/descriptors/xinput.h"
#include "usb/descriptors/xboxog_sb.h"
#include "usb/device/device_private.h"

#define DEFAULT_SENSITIVITY ((uint16_t)400)
#define DEFAULT_DEADZONE    ((int16_t)7500)

typedef struct {
    uint32_t gp_btn;
    uint16_t sb_btn;
    uint8_t  sb_btn_off;
} xbsb_button_map_t;

static const xbsb_button_map_t GP_MAP[] = {
    { GAMEPAD_BUTTON_START, XBSB_BTN0_START,              0 },
    { GAMEPAD_BUTTON_LB,    XBSB_BTN0_RIGHTJOYFIRE,       0 },
    { GAMEPAD_BUTTON_R3,    XBSB_BTN0_RIGHTJOYLOCKON,     0 },
    { GAMEPAD_BUTTON_B,     XBSB_BTN0_RIGHTJOYLOCKON,     0 },
    { GAMEPAD_BUTTON_RB,    XBSB_BTN0_RIGHTJOYMAINWEAPON, 0 },
    { GAMEPAD_BUTTON_A,     XBSB_BTN0_RIGHTJOYMAINWEAPON, 0 },
    { GAMEPAD_BUTTON_SYS,   XBSB_BTN0_EJECT,              0 },
    { GAMEPAD_BUTTON_L3,    XBSB_BTN2_LEFTJOYSIGHTCHANGE, 2 },
    { GAMEPAD_BUTTON_Y,     XBSB_BTN1_CHAFF,              1 }
};

static const xbsb_button_map_t CHATPAD_MAP[] = {
    { XINPUT_KEYCODE_0,     XBSB_BTN0_EJECT,                0 },
    { XINPUT_KEYCODE_D,     XBSB_BTN1_WASHING,              1 },
    { XINPUT_KEYCODE_F,     XBSB_BTN1_EXTINGUISHER,         1 },
    { XINPUT_KEYCODE_G,     XBSB_BTN1_CHAFF,                1 },
    { XINPUT_KEYCODE_X,     XBSB_BTN1_WEAPONCONMAIN,        1 },
    { XINPUT_KEYCODE_RIGHT, XBSB_BTN1_WEAPONCONMAIN,        1 },
    { XINPUT_KEYCODE_C,     XBSB_BTN1_WEAPONCONSUB,         1 },
    { XINPUT_KEYCODE_LEFT,  XBSB_BTN1_WEAPONCONSUB,         1 },
    { XINPUT_KEYCODE_V,     XBSB_BTN1_WEAPONCONMAGAZINE,    1 },
    { XINPUT_KEYCODE_SPACE, XBSB_BTN1_WEAPONCONMAGAZINE,    1 },
    { XINPUT_KEYCODE_U,     XBSB_BTN0_MULTIMONOPENCLOSE,    0 },
    { XINPUT_KEYCODE_J,     XBSB_BTN0_MULTIMONMODESELECT,   0 },
    { XINPUT_KEYCODE_N,     XBSB_BTN0_MAINMONZOOMIN,        0 },
    { XINPUT_KEYCODE_I,     XBSB_BTN0_MULTIMONMAPZOOMINOUT, 0 },
    { XINPUT_KEYCODE_K,     XBSB_BTN0_MULTIMONSUBMONITOR,   0 },
    { XINPUT_KEYCODE_M,     XBSB_BTN0_MAINMONZOOMOUT,       0 },
    { XINPUT_KEYCODE_ENTER, XBSB_BTN0_START,                0 },
    { XINPUT_KEYCODE_P,     XBSB_BTN0_COCKPITHATCH,         0 },
    { XINPUT_KEYCODE_COMMA, XBSB_BTN0_IGNITION,             0 }
};

static const xbsb_button_map_t CHATPAD_MAP_ALT1[] = {
    { XINPUT_KEYCODE_1, XBSB_BTN1_COMM1, 1 },
    { XINPUT_KEYCODE_2, XBSB_BTN1_COMM2, 1 },
    { XINPUT_KEYCODE_3, XBSB_BTN1_COMM3, 1 },
    { XINPUT_KEYCODE_4, XBSB_BTN1_COMM4, 1 },
    { XINPUT_KEYCODE_5, XBSB_BTN2_COMM5, 2 }
};

static const xbsb_button_map_t CHATPAD_MAP_ALT2[] = {
    { XINPUT_KEYCODE_1, XBSB_BTN1_FUNCTIONF1,              1 },
    { XINPUT_KEYCODE_2, XBSB_BTN1_FUNCTIONTANKDETACH,      1 },
    { XINPUT_KEYCODE_3, XBSB_BTN0_FUNCTIONFSS,             0 },
    { XINPUT_KEYCODE_4, XBSB_BTN1_FUNCTIONF2,              1 },
    { XINPUT_KEYCODE_5, XBSB_BTN1_FUNCTIONOVERRIDE,        1 },
    { XINPUT_KEYCODE_6, XBSB_BTN0_FUNCTIONMANIPULATOR,     0 },
    { XINPUT_KEYCODE_7, XBSB_BTN1_FUNCTIONF3,              1 },
    { XINPUT_KEYCODE_8, XBSB_BTN1_FUNCTIONNIGHTSCOPE,      1 },
    { XINPUT_KEYCODE_9, XBSB_BTN0_FUNCTIONLINECOLORCHANGE, 0 }
};

static const xbsb_button_map_t CHATPAD_TOGGLE_MAP[] = {
    { XINPUT_KEYCODE_Q, XBSB_BTN2_TOGGLEOXYGENSUPPLY,   2 },
    { XINPUT_KEYCODE_A, XBSB_BTN2_TOGGLEFILTERCONTROL,  2 },
    { XINPUT_KEYCODE_W, XBSB_BTN2_TOGGLEVTLOCATION,     2 },
    { XINPUT_KEYCODE_S, XBSB_BTN2_TOGGLEBUFFREMATERIAL, 2 },
    { XINPUT_KEYCODE_Z, XBSB_BTN2_TOGGLEFUELFLOWRATE,   2 }
};

typedef struct {
    xbsb_report_in_t    report_in;
    xbsb_report_out_t   report_out;
    gamepad_rumble_t    gp_rumble;
    gamepad_pad_t       gp_pad;

    bool                dpad_reset;
    bool                toggle_pressed[ARRAY_SIZE(CHATPAD_TOGGLE_MAP)];
    bool                shift_pressed;
    uint16_t            sensitivity;
    uint32_t            aim_reset_timer;
    int32_t             vmouse_x;
    int32_t             vmouse_y;
} xbsb_state_t;
_Static_assert(sizeof(xbsb_state_t) <= USBD_STATUS_BUF_SIZE, "XBOXOG GP state size exceeds buffer size");

static xbsb_state_t *xbsb_state[USBD_DEVICES_MAX] = { NULL };

static inline bool chatpad_pressed(const uint8_t chatpad[3], uint16_t keycode) {
    if (chatpad[0] == 0 && chatpad[1] == 0 && chatpad[2] == 0) {
        return false;
    } else if (keycode < 17 && (chatpad[0] & keycode)) {
        return true;
    } else if (keycode < 17) {
        return false;
    } else if (chatpad[1] == keycode) {
        return true;
    } else if (chatpad[2] == keycode) {
        return true;
    }
    return false;
}

static void xbsb_init_cb(usbd_handle_t *handle) { (void)handle; }

static void xbsb_deinit_cb(usbd_handle_t *handle) { (void)handle; }

static bool xbsb_get_desc_cb(usbd_handle_t *handle, const usb_ctrl_req_t *req) {
    const uint8_t type  = req->wValue & 0xFF;
    // const uint8_t index = req->wValue >> 8;
    switch (type) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &XBSB_DESC_DEVICE,
                                   sizeof(XBSB_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &XBSB_DESC_CONFIG,
                                   sizeof(XBSB_DESC_CONFIG), NULL);
    case USB_DTYPE_XID:
        return usbd_send_ctrl_resp(handle, &XBSB_DESC_XID,
                                   sizeof(XBSB_DESC_XID), NULL);
    default:
        break;
    }
    return false;
}

static bool xbsb_ctrl_xfer_cb(usbd_handle_t *handle,
                              const usb_ctrl_req_t *req) {
    xbsb_state_t *xbsb = xbsb_state[handle->port];
    switch (req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) {
    case (USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE): {
        // const uint8_t itf_num     = req->wIndex & 0xFF;
        // const uint8_t report_id   = req->wValue >> 8;
        // const uint8_t report_type = req->wValue & 0xFF;
        switch (req->bRequest) {
        case USB_REQ_HID_GET_REPORT:
            return usbd_send_ctrl_resp(handle, &xbsb->report_in,
                                       sizeof(xbsb->report_in), NULL);
        case USB_REQ_HID_SET_REPORT:
            return true;
        default:
            break;
        }
    } break;
    case (USB_REQ_TYPE_VENDOR | USB_REQ_RECIP_INTERFACE):
        if (req->bRequest == USB_REQ_XID_GET_CAPABILITIES) {
            const uint8_t report_id = req->wValue >> 8;
            switch (report_id) {
            case USB_XID_REPORT_CAPABILITIES_IN: 
                return usbd_send_ctrl_resp(handle, &XBOXOG_SB_CAPABILITIES_IN,
                                           sizeof(XBOXOG_SB_CAPABILITIES_IN), NULL);
            case USB_XID_REPORT_CAPABILITIES_OUT: 
                return usbd_send_ctrl_resp(handle, &XBOXOG_SB_CAPABILITIES_OUT,
                                           sizeof(XBOXOG_SB_CAPABILITIES_OUT), NULL);
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    return false;
}

static bool xbsb_set_config_cb(usbd_handle_t *handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &XBSB_DESC_CONFIG);
}

static void xbsb_configured_cb(usbd_handle_t *handle, uint8_t config) {
    (void)config;
    xbsb_state_t *xbsb = xbsb_state[handle->port];
    xbsb->vmouse_x = XBSB_AIMING_MID;
    xbsb->vmouse_y = XBSB_AIMING_MID;
    xbsb->sensitivity = DEFAULT_SENSITIVITY;
    xbsb->aim_reset_timer = 0;
    xbsb->dpad_reset = true;

    memset(&xbsb->report_in, 0, sizeof(xbsb->report_in));
    xbsb->report_in.bLength = sizeof(xbsb->report_in);
    xbsb->report_in.gearLever = XBSB_GEAR_N;
}

static void xbsb_ep_xfer_cb(usbd_handle_t *handle, uint8_t epaddr) {
    xbsb_state_t* xbsb = xbsb_state[handle->port];
    if (epaddr == XBSB_EPADDR_OUT) {
        int32_t len = usbd_ep_read(handle, XBSB_EPADDR_OUT,
                                   &xbsb->report_out, sizeof(xbsb->report_out));
        if (len != sizeof(xbsb->report_out)) {
            return;
        }
        xbsb->gp_rumble.l  = xbsb->report_out.Chaff_Extinguisher;
        xbsb->gp_rumble.l |= xbsb->report_out.Chaff_Extinguisher << 4;
        xbsb->gp_rumble.l |= xbsb->report_out.Comm1_MagazineChange << 4;
        xbsb->gp_rumble.l |= xbsb->report_out.CockpitHatch_EmergencyEject << 4;
        xbsb->gp_rumble.r  = xbsb->gp_rumble.l;
        usb_device_rumble_cb(handle, &xbsb->gp_rumble);
    }
}

static usbd_handle_t *xboxog_sb_init(const usb_device_driver_cfg_t *cfg) {
    if ((cfg == NULL) || (cfg->usb.status_buffer == NULL)) {
        return NULL;
    }
    usbd_driver_t driver = {
        .init_cb       = xbsb_init_cb,
        .deinit_cb     = xbsb_deinit_cb,
        .get_desc_cb   = xbsb_get_desc_cb,
        .set_config_cb = xbsb_set_config_cb,
        .configured_cb = xbsb_configured_cb,
        .ctrl_xfer_cb  = xbsb_ctrl_xfer_cb,
        .ep_xfer_cb    = xbsb_ep_xfer_cb,
    };
    usbd_handle_t *handle =
        usbd_init(cfg->usb.hw_type, &driver, XBSB_EPSIZE_CTRL);
    if (handle != NULL) {
        xbsb_state[handle->port] = (xbsb_state_t *)cfg->usb.status_buffer;
    }
    return handle;
}

static void xboxog_sb_set_pad(usbd_handle_t* handle, const gamepad_pad_t* pad) {
    if (!usbd_ep_ready(handle, XBSB_EPADDR_IN)) {
        return;
    }
    xbsb_state_t *xbsb = xbsb_state[handle->port];

    xbsb->report_in.dButtons[0]  = 0;
    xbsb->report_in.dButtons[1]  = 0;
    xbsb->report_in.dButtons[2] &= XBSB_BTN2_TOGGLE_MID;

    for (uint8_t i = 0; i < ARRAY_SIZE(GP_MAP); i++) {
        if (pad->buttons & GP_MAP[i].gp_btn) {
            xbsb->report_in.dButtons[GP_MAP[i].sb_btn_off] |= GP_MAP[i].sb_btn;
        }
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(CHATPAD_MAP); i++) {
        if (chatpad_pressed(pad->chatpad, CHATPAD_MAP[i].gp_btn)) {
            xbsb->report_in.dButtons[CHATPAD_MAP[i].sb_btn_off] |= CHATPAD_MAP[i].sb_btn;
        }
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(CHATPAD_TOGGLE_MAP); i++) {
        if (chatpad_pressed(pad->chatpad, CHATPAD_TOGGLE_MAP[i].gp_btn)) {
            if (!xbsb->toggle_pressed[i]) {
                xbsb->report_in.dButtons[CHATPAD_TOGGLE_MAP[i].sb_btn_off] ^=
                    CHATPAD_TOGGLE_MAP[i].sb_btn;
                xbsb->toggle_pressed[i] = true;
            }
        } else {
            xbsb->toggle_pressed[i] = false;
        }
    }

    if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_SHIFT)) {
        if (!xbsb->shift_pressed) {
            if (xbsb->report_in.dButtons[2] & XBSB_BTN2_TOGGLE_MID) {
                xbsb->report_in.dButtons[2] &= ~XBSB_BTN2_TOGGLE_MID;
            } else {
                xbsb->report_in.dButtons[2] |= XBSB_BTN2_TOGGLE_MID;
            }
            xbsb->shift_pressed = true;
        }
    } else {
        xbsb->shift_pressed = false;
    }

    if (pad->buttons & GAMEPAD_BUTTON_X) {
        if (xbsb->report_out.Chaff_Extinguisher & 0x0F) {
            xbsb->report_in.dButtons[1] |= XBSB_BTN1_EXTINGUISHER;
        }
        if (xbsb->report_out.Comm1_MagazineChange & 0x0F) {
            xbsb->report_in.dButtons[1] |=
                XBSB_BTN1_WEAPONCONMAGAZINE;
        }
        if (xbsb->report_out.Washing_LineColorChange & 0xF0) {
            xbsb->report_in.dButtons[1] |= XBSB_BTN1_WASHING;
        }
    }

    if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_MESSENGER) ||
        pad->buttons & GAMEPAD_BUTTON_BACK) {
        for (uint8_t i = 0; i < ARRAY_SIZE(CHATPAD_MAP_ALT1); i++) {
            if (chatpad_pressed(pad->chatpad, CHATPAD_MAP_ALT1[i].gp_btn)) {
                xbsb->report_in.dButtons[CHATPAD_MAP_ALT1[i].sb_btn_off] |= CHATPAD_MAP_ALT1[i].sb_btn;
            }
        }

        if (pad->dpad & GAMEPAD_DPAD_UP && xbsb->dpad_reset) {
            xbsb->report_in.tunerDial = (xbsb->report_in.tunerDial + 1) % 16;
            xbsb->dpad_reset = false;
        } else if (pad->dpad & GAMEPAD_DPAD_DOWN && xbsb->dpad_reset) {
            xbsb->report_in.tunerDial = (xbsb->report_in.tunerDial + 15) % 16;
            xbsb->dpad_reset = false;
        } else if (!(pad->dpad & GAMEPAD_DPAD_DOWN) &&
                   !(pad->dpad & GAMEPAD_DPAD_UP)) {
            xbsb->dpad_reset = true;
        }
    } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_ORANGE)) {
        for (uint8_t i = 0; i < ARRAY_SIZE(CHATPAD_MAP_ALT2); i++) {
            if (chatpad_pressed(pad->chatpad, CHATPAD_MAP_ALT2[i].gp_btn)) {
                xbsb->report_in.dButtons[CHATPAD_MAP_ALT2[i].sb_btn_off] |= CHATPAD_MAP_ALT2[i].sb_btn;
            }
        }

        // if (!(gp->dpad & GAMEPAD_DPAD_LEFT) && !(gp->dpad &
        // GAMEPAD_DPAD_RIGHT))
        // {
        if (pad->dpad & GAMEPAD_DPAD_UP && xbsb->dpad_reset) {
            if (xbsb->report_in.gearLever < XBSB_GEAR_5) {
                xbsb->report_in.gearLever++;
            }
            xbsb->dpad_reset = false;
        } else if (pad->dpad & GAMEPAD_DPAD_DOWN && xbsb->dpad_reset) {
            if (xbsb->report_in.gearLever > XBSB_GEAR_R) {
                xbsb->report_in.gearLever--;
            }
            xbsb->dpad_reset = false;
        } else if (!(pad->dpad & GAMEPAD_DPAD_DOWN) &&
                   !(pad->dpad & GAMEPAD_DPAD_UP)) {
            xbsb->dpad_reset = true;
        }
        // }
    } else {
        xbsb->dpad_reset = true;
    }

    xbsb->report_in.leftPedal  = range_uint8_to_uint16(pad->trigger_l);
    xbsb->report_in.rightPedal = range_uint8_to_uint16(pad->trigger_r);
    xbsb->report_in.middlePedal =
        chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_BACK) ? 0xFF00 : 0x0000;
    xbsb->report_in.rotationLever =
        chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_MESSENGER) 
        ? 0 : (pad->buttons & GAMEPAD_BUTTON_BACK)                   
            ? 0 : (pad->dpad & GAMEPAD_DPAD_LEFT)  
                ? R_INT16_MIN : (pad->dpad & GAMEPAD_DPAD_RIGHT) 
                    ? R_INT16_MAX : 0;

    xbsb->report_in.sightChangeX = pad->joystick_lx;
    xbsb->report_in.sightChangeY = pad->joystick_ly;

    int32_t axis_value_x = (int32_t)pad->joystick_rx;
    if (abs(axis_value_x) > DEFAULT_DEADZONE) {
        xbsb->vmouse_x += axis_value_x / xbsb->sensitivity;
    }

    int32_t axis_value_y = (int32_t)range_invert_int16(pad->joystick_ry);
    if (abs(axis_value_y) > DEFAULT_DEADZONE) {
        xbsb->vmouse_y -= axis_value_y / xbsb->sensitivity;
    }

    if (xbsb->vmouse_x < 0) {
        xbsb->vmouse_x = 0;
    }
    if (xbsb->vmouse_x > R_UINT16_MAX) {
        xbsb->vmouse_x = R_UINT16_MAX;
    }
    if (xbsb->vmouse_y > R_UINT16_MAX) {
        xbsb->vmouse_y = R_UINT16_MAX;
    }
    if (xbsb->vmouse_y < 0) {
        xbsb->vmouse_y = 0;
    }

    if (pad->buttons & GAMEPAD_BUTTON_L3) {
        if ((time_us_32() / 1000) - xbsb->aim_reset_timer > 500) {
            xbsb->vmouse_x = XBSB_AIMING_MID;
            xbsb->vmouse_y = XBSB_AIMING_MID;
        }
    } else {
        xbsb->aim_reset_timer = time_us_32() / 1000;
    }

    xbsb->report_in.aimingX = (uint16_t)xbsb->vmouse_x;
    xbsb->report_in.aimingY = (uint16_t)xbsb->vmouse_y;

    usbd_ep_write(handle, XBSB_EPADDR_IN, &xbsb->report_in, sizeof(xbsb->report_in));

    if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_ORANGE)) {
        if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_9)) {
            xbsb->sensitivity = 200;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_8)) {
            xbsb->sensitivity = 250;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_7)) {
            xbsb->sensitivity = 300;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_6)) {
            xbsb->sensitivity = 350;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_5)) {
            xbsb->sensitivity = 400;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_4)) {
            xbsb->sensitivity = 650;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_3)) {
            xbsb->sensitivity = 800;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_2)) {
            xbsb->sensitivity = 1000;
        } else if (chatpad_pressed(pad->chatpad, XINPUT_KEYCODE_1)) {
            xbsb->sensitivity = 1200;
        }
    }
}

const usb_device_driver_t USBD_DRIVER_XBOXOG_SB = {
    .name   = "Xbox OG Steel Battalion",
    .init   = xboxog_sb_init,
    .task   = NULL,
    .set_audio = NULL,
    .set_pad = xboxog_sb_set_pad
};