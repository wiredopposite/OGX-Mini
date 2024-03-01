// Copyright 2020, Ryan Wendland, usb64
// SPDX-License-Identifier: MIT

#include "tusb_option.h"

#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_XINPUT)

// #include "host/usbh.h"
// #include "host/usbh_pvt.h"
#include "usbh/tusb_xinput/xinput_host.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"

//Wired 360 commands
static const uint8_t xbox360_wired_rumble[] = {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xbox360_wired_led[] = {0x01, 0x03, 0x00};

//Xbone one
#define GIP_CMD_ACK 0x01
#define GIP_CMD_ANNOUNCE 0x02
#define GIP_CMD_IDENTIFY 0x04
#define GIP_CMD_POWER 0x05
#define GIP_CMD_AUTHENTICATE 0x06
#define GIP_CMD_VIRTUAL_KEY 0x07
#define GIP_CMD_RUMBLE 0x09
#define GIP_CMD_LED 0x0a
#define GIP_CMD_FIRMWARE 0x0c
#define GIP_CMD_INPUT 0x20
#define GIP_SEQ0 0x00
#define GIP_OPT_ACK 0x10
#define GIP_OPT_INTERNAL 0x20
#define GIP_PL_LEN(N) (N)
#define GIP_PWR_ON 0x00
#define GIP_PWR_SLEEP 0x01
#define GIP_PWR_OFF 0x04
#define GIP_PWR_RESET 0x07
#define GIP_LED_ON 0x01
#define BIT(n) (1UL << (n))
#define GIP_MOTOR_R  BIT(0)
#define GIP_MOTOR_L  BIT(1)
#define GIP_MOTOR_RT BIT(2)
#define GIP_MOTOR_LT BIT(3)
#define GIP_MOTOR_ALL (GIP_MOTOR_R | GIP_MOTOR_L | GIP_MOTOR_RT | GIP_MOTOR_LT)
static const uint8_t xboxone_power_on[] = {GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(1), GIP_PWR_ON};
static const uint8_t xboxone_s_init[] = {GIP_CMD_POWER, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(15), 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t xboxone_s_led_init[] = {GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(3), 0x00, 0x01, 0x14};
static const uint8_t extra_input_packet_init[] = {0x4d, 0x10, GIP_SEQ0, 0x02, 0x07, 0x00};
static const uint8_t xboxone_pdp_led_on[] = {GIP_CMD_LED, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(3), 0x00, GIP_LED_ON, 0x14};
static const uint8_t xboxone_pdp_auth[] = {GIP_CMD_AUTHENTICATE, GIP_OPT_INTERNAL, GIP_SEQ0, GIP_PL_LEN(2), 0x01, 0x00};
static const uint8_t xboxone_rumble[] = {GIP_CMD_RUMBLE, 0x00, 0x00, GIP_PL_LEN(9), 0x00, GIP_MOTOR_ALL, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF};

//Wireless 360 commands
static const uint8_t xbox360w_led[] = {0x00, 0x00, 0x08, 0x40};
//Sending 0x00, 0x00, 0x08, 0x00 will permanently disable rumble until you do this:
static const uint8_t xbox360w_rumble_enable[] = {0x00, 0x00, 0x08, 0x01};
static const uint8_t xbox360w_rumble[] = {0x00, 0x01, 0x0F, 0xC0, 0x00, 0x00};
static const uint8_t xbox360w_inquire_present[] = {0x08, 0x00, 0x0F, 0xC0};
static const uint8_t xbox360w_controller_info[] = {0x00, 0x00, 0x00, 0x40};
static const uint8_t xbox360w_unknown[] = {0x00, 0x00, 0x02, 0x80};
static const uint8_t xbox360w_power_off[] = {0x00, 0x00, 0x08, 0xC0};
static const uint8_t xbox360w_chatpad_init[] = {0x00, 0x00, 0x0C, 0x1B};
static const uint8_t xbox360w_chatpad_keepalive1[] = {0x00, 0x00, 0x0C, 0x1F};
static const uint8_t xbox360w_chatpad_keepalive2[] = {0x00, 0x00, 0x0C, 0x1E};

//Original Xbox
static const uint8_t xboxog_rumble[] = {0x00, 0x06, 0x00, 0x00, 0x00, 0x00};

#pragma GCC diagnostic pop

typedef struct
{
    uint8_t inst_count;
    xinputh_interface_t instances[CFG_TUH_XINPUT];
} xinputh_device_t;

static xinputh_device_t _xinputh_dev[CFG_TUH_DEVICE_MAX];

TU_ATTR_ALWAYS_INLINE static inline xinputh_device_t *get_dev(uint8_t dev_addr)
{
    return &_xinputh_dev[dev_addr - 1];
}

TU_ATTR_ALWAYS_INLINE static inline xinputh_interface_t *get_instance(uint8_t dev_addr, uint8_t instance)
{
    return &_xinputh_dev[dev_addr - 1].instances[instance];
}

static uint8_t get_instance_id_by_epaddr(uint8_t dev_addr, uint8_t ep_addr)
{
    for (uint8_t inst = 0; inst < CFG_TUH_XINPUT; inst++)
    {
        xinputh_interface_t *hid = get_instance(dev_addr, inst);

        if ((ep_addr == hid->ep_in) || (ep_addr == hid->ep_out))
            return inst;
    }

    return 0xff;
}

static uint8_t get_instance_id_by_itfnum(uint8_t dev_addr, uint8_t itf)
{
    for (uint8_t inst = 0; inst < CFG_TUH_XINPUT; inst++)
    {
        xinputh_interface_t *hid = get_instance(dev_addr, inst);

        if ((hid->itf_num == itf) && (hid->ep_in || hid->ep_out))
            return inst;
    }

    return 0xff;
}

static void wait_for_tx_complete(uint8_t dev_addr, uint8_t ep_out)
{
    while (usbh_edpt_busy(dev_addr, ep_out))
        tuh_task();
}

static void xboxone_init( xinputh_interface_t *xid_itf, uint8_t dev_addr, uint8_t instance)
{
    uint16_t PID, VID;
    tuh_vid_pid_get(dev_addr, &VID, &PID);

    tuh_xinput_send_report(dev_addr, instance, xboxone_power_on, sizeof(xboxone_power_on));
    wait_for_tx_complete(dev_addr, xid_itf->ep_out);
    tuh_xinput_send_report(dev_addr, instance, xboxone_s_init, sizeof(xboxone_s_init));
    wait_for_tx_complete(dev_addr, xid_itf->ep_out);

    if (VID == 0x045e && (PID == 0x0b00))
    {
        tuh_xinput_send_report(dev_addr, instance, extra_input_packet_init, sizeof(extra_input_packet_init));
        wait_for_tx_complete(dev_addr, xid_itf->ep_out);
    }

    //Required for PDP aftermarket controllers
    if (VID == 0x0e6f)
    {
        tuh_xinput_send_report(dev_addr, instance, xboxone_pdp_led_on, sizeof(xboxone_pdp_led_on));
        wait_for_tx_complete(dev_addr, xid_itf->ep_out);
        tuh_xinput_send_report(dev_addr, instance, xboxone_pdp_auth, sizeof(xboxone_pdp_auth));
        wait_for_tx_complete(dev_addr, xid_itf->ep_out);
    }
}

bool tuh_xinput_receive_report(uint8_t dev_addr, uint8_t instance)
{
    xinputh_interface_t *xid_itf = get_instance(dev_addr, instance);
    TU_VERIFY(usbh_edpt_claim(dev_addr, xid_itf->ep_in));

    if ( !usbh_edpt_xfer(dev_addr, xid_itf->ep_in, xid_itf->epin_buf, xid_itf->epin_size) )
    {
        usbh_edpt_release(dev_addr, xid_itf->ep_in);
        return false;
    }
    return true;
}

bool tuh_xinput_send_report(uint8_t dev_addr, uint8_t instance, const uint8_t *txbuf, uint16_t len)
{
    xinputh_interface_t *xid_itf = get_instance(dev_addr, instance);

    TU_ASSERT(len <= xid_itf->epout_size);
    TU_VERIFY(usbh_edpt_claim(dev_addr, xid_itf->ep_out));

    memcpy(xid_itf->epout_buf, txbuf, len);

    if ( !usbh_edpt_xfer(dev_addr, xid_itf->ep_out, xid_itf->epout_buf, len))
    {
        usbh_edpt_release(dev_addr, xid_itf->ep_out);
        return false;
    }
    return true;
}

bool tuh_xinput_set_led(uint8_t dev_addr, uint8_t instance, uint8_t quadrant, bool block)
{
    xinputh_interface_t *xid_itf = get_instance(dev_addr, instance);
    uint8_t txbuf[32];
    uint16_t len;
    switch (xid_itf->type)
    {
    case XBOX360_WIRELESS:
        memcpy(txbuf, xbox360w_led, sizeof(xbox360w_led));
        txbuf[3] = (quadrant == 0) ? 0x40 : (0x40 | (quadrant + 5));
        len = sizeof(xbox360w_led);
        break;
    case XBOX360_WIRED:
        memcpy(txbuf, xbox360_wired_led, sizeof(xbox360_wired_led));
        txbuf[2] = (quadrant == 0) ? 0 : (quadrant + 5);
        len = sizeof(xbox360_wired_led);
        break;
    default:
        return true;
    }
    bool ret = tuh_xinput_send_report(dev_addr, instance, txbuf, len);
    if (block && ret)
    {
        wait_for_tx_complete(dev_addr, xid_itf->ep_out);
    }
    return ret;
}

bool tuh_xinput_set_rumble(uint8_t dev_addr, uint8_t instance, uint8_t lValue, uint8_t rValue, bool block)
{
    xinputh_interface_t *xid_itf = get_instance(dev_addr, instance);
    uint8_t txbuf[32];
    uint16_t len;

    switch (xid_itf->type)
    {
    case XBOX360_WIRELESS:
        memcpy(txbuf, xbox360w_rumble, sizeof(xbox360w_rumble));
        txbuf[5] = lValue;
        txbuf[6] = rValue;
        len = sizeof(xbox360w_rumble);
        break;
    case XBOX360_WIRED:
        memcpy(txbuf, xbox360_wired_rumble, sizeof(xbox360_wired_rumble));
        txbuf[3] = lValue;
        txbuf[4] = rValue;
        len = sizeof(xbox360_wired_rumble);
        break;
    case XBOXONE:
        memcpy(txbuf, xboxone_rumble, sizeof(xboxone_rumble));
        txbuf[8] = lValue / 2; // 0 - 128
        txbuf[9] = rValue / 2; // 0 - 128
        len = sizeof(xboxone_rumble);
        break;
    case XBOXOG:
        memcpy(txbuf, xboxog_rumble, sizeof(xboxog_rumble));
        txbuf[2] = lValue;
        txbuf[3] = lValue;
        txbuf[4] = rValue;
        txbuf[5] = rValue;
        len = sizeof(xboxog_rumble);
        break;
    default:
        return true;
    }
    bool ret = tuh_xinput_send_report(dev_addr, instance, txbuf, len);
    if (block && ret)
    {
        wait_for_tx_complete(dev_addr, xid_itf->ep_out);
    }
    return true;
}

//--------------------------------------------------------------------+
// USBH API
//--------------------------------------------------------------------+
void xinputh_init(void)
{
    tu_memclr(_xinputh_dev, sizeof(_xinputh_dev));
}

bool xinputh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX);

    xinput_type_t type = XINPUT_UNKNOWN;
    if (desc_itf->bNumEndpoints < 2)
        type = XINPUT_UNKNOWN;
    else if (desc_itf->bInterfaceSubClass == 0x5D && //Xbox360 wireless bInterfaceSubClass
             desc_itf->bInterfaceProtocol == 0x81)   //Xbox360 wireless bInterfaceProtocol
        type = XBOX360_WIRELESS;
    else if (desc_itf->bInterfaceSubClass == 0x5D && //Xbox360 wired bInterfaceSubClass
             desc_itf->bInterfaceProtocol == 0x01)   //Xbox360 wired bInterfaceProtocol
        type = XBOX360_WIRED;
    else if (desc_itf->bInterfaceSubClass == 0x47 && //Xbone and SX bInterfaceSubClass
             desc_itf->bInterfaceProtocol == 0xD0)   //Xbone and SX bInterfaceProtocol
        type = XBOXONE;
    else if (desc_itf->bInterfaceClass == 0x58 &&  //XboxOG bInterfaceClass
             desc_itf->bInterfaceSubClass == 0x42) //XboxOG bInterfaceSubClass
        type = XBOXOG;

    if (type == XINPUT_UNKNOWN)
    {
        TU_LOG2("XINPUT: Not a valid interface\n");
        return false;
    }

    TU_LOG2("XINPUT opening Interface %u (addr = %u)\r\n", desc_itf->bInterfaceNumber, dev_addr);

    xinputh_device_t *xinput_dev = get_dev(dev_addr);
    TU_ASSERT(xinput_dev->inst_count < CFG_TUH_XINPUT, 0);

    xinputh_interface_t *xid_itf = get_instance(dev_addr, xinput_dev->inst_count);
    xid_itf->itf_num = desc_itf->bInterfaceNumber;
    xid_itf->type = type;

    //Parse descriptor for all endpoints and open them
    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    int endpoint = 0;
    int pos = 0;
    while (endpoint < desc_itf->bNumEndpoints && pos < max_len)
    {
        if (tu_desc_type(p_desc) != TUSB_DESC_ENDPOINT)
        {
            pos += tu_desc_len(p_desc);
            p_desc = tu_desc_next(p_desc);
            continue;
        }
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
        TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
        if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT)
        {
            xid_itf->ep_out = desc_ep->bEndpointAddress;
            xid_itf->epout_size = tu_edpt_packet_size(desc_ep);
        }
        else
        {
            xid_itf->ep_in = desc_ep->bEndpointAddress;
            xid_itf->epin_size = tu_edpt_packet_size(desc_ep);
        }
        endpoint++;
        pos += tu_desc_len(p_desc);
        p_desc = tu_desc_next(p_desc);
    }

    xinput_dev->inst_count++;
    return true;
}

bool xinputh_set_config(uint8_t dev_addr, uint8_t itf_num)
{
    uint8_t instance = get_instance_id_by_itfnum(dev_addr, itf_num);
    xinputh_interface_t *xid_itf = get_instance(dev_addr, instance);
    xid_itf->connected = true;

    if (xid_itf->type == XBOX360_WIRELESS)
    {
        //Wireless controllers may not be connected yet.
        xid_itf->connected = false;
        tuh_xinput_send_report(dev_addr, instance, xbox360w_inquire_present, sizeof(xbox360w_inquire_present));
        wait_for_tx_complete(dev_addr, xid_itf->ep_out);
    }
    else if (xid_itf->type == XBOX360_WIRED)
    {
    }
    else if (xid_itf->type == XBOXONE)
    {
        xboxone_init(xid_itf, dev_addr, instance);
    }

    if (tuh_xinput_mount_cb)
    {
        tuh_xinput_mount_cb(dev_addr, instance, xid_itf);
    }

    usbh_driver_set_config_complete(dev_addr, xid_itf->itf_num);
    return true;
}

bool xinputh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    uint8_t const dir = tu_edpt_dir(ep_addr);
    uint8_t const instance = get_instance_id_by_epaddr(dev_addr, ep_addr);
    xinputh_interface_t *xid_itf = get_instance(dev_addr, instance);
    xinput_gamepad_t *pad = &xid_itf->pad;
    uint8_t *rdata = xid_itf->epin_buf;

    xid_itf->last_xfer_result = result;
    xid_itf->last_xferred_bytes = xferred_bytes;

    // On transfer error, bail early but notify the application
    if (result != XFER_RESULT_SUCCESS)
    {
        if (dir == TUSB_DIR_IN)
        {
            tuh_xinput_report_received_cb(dev_addr, instance, xid_itf, sizeof(xinputh_interface_t));
        }
        else if (tuh_xinput_report_sent_cb)
        {
            tuh_xinput_report_sent_cb(dev_addr, instance, xid_itf->epout_buf, xferred_bytes);
        }
        return false;
    }

    if (dir == TUSB_DIR_IN)
    {
        TU_LOG2("Get Report callback (%u, %u, %u bytes)\r\n", dev_addr, instance, xferred_bytes);
        if (xid_itf->type == XBOX360_WIRED)
        {
            #define GET_USHORT(a) (uint16_t)((a)[1] << 8 | (a)[0])
            #define GET_SHORT(a) ((int16_t)GET_USHORT(a))
            if (rdata[1] == 0x14)
            {
                tu_memclr(pad, sizeof(xinput_gamepad_t));
                uint16_t wButtons = rdata[3] << 8 | rdata[2];

                //Map digital buttons
                if (wButtons & (1 << 0)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_UP;
                if (wButtons & (1 << 1)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
                if (wButtons & (1 << 2)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
                if (wButtons & (1 << 3)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
                if (wButtons & (1 << 4)) pad->wButtons |= XINPUT_GAMEPAD_START;
                if (wButtons & (1 << 5)) pad->wButtons |= XINPUT_GAMEPAD_BACK;
                if (wButtons & (1 << 6)) pad->wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
                if (wButtons & (1 << 7)) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
                if (wButtons & (1 << 8)) pad->wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
                if (wButtons & (1 << 9)) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
                if (wButtons & (1 << 10)) pad->wButtons |= XINPUT_GAMEPAD_GUIDE;
                if (wButtons & (1 << 12)) pad->wButtons |= XINPUT_GAMEPAD_A;
                if (wButtons & (1 << 13)) pad->wButtons |= XINPUT_GAMEPAD_B;
                if (wButtons & (1 << 14)) pad->wButtons |= XINPUT_GAMEPAD_X;
                if (wButtons & (1 << 15)) pad->wButtons |= XINPUT_GAMEPAD_Y;

                //Map the left and right triggers
                pad->bLeftTrigger = rdata[4];
                pad->bRightTrigger = rdata[5];

                //Map analog sticks
                pad->sThumbLX = rdata[7] << 8 | rdata[6];
                pad->sThumbLY = rdata[9] << 8 | rdata[8];
                pad->sThumbRX = rdata[11] << 8 | rdata[10];
                pad->sThumbRY = rdata[13] << 8 | rdata[12];

                xid_itf->new_pad_data = true;
            }           
        }
        else if (xid_itf->type == XBOX360_WIRELESS)
        {
            //Connect/Disconnect packet
            if (rdata[0] & 0x08)
            {
                if (rdata[1] != 0x00 && xid_itf->connected == false)
                {
                    TU_LOG2("XINPUT: WIRELESS CONTROLLER CONNECTED\n");
                    xid_itf->connected = true;
                }
                else if (rdata[1] == 0x00 && xid_itf->connected == true)
                {
                    TU_LOG2("XINPUT: WIRELESS CONTROLLER DISCONNECTED\n");
                    xid_itf->connected = false;
                }
            }

            //Button status packet
            if ((rdata[1] & 1) && rdata[5] == 0x13)
            {
                tu_memclr(pad, sizeof(xinput_gamepad_t));
                uint16_t wButtons = rdata[7] << 8 | rdata[6];

                //Map digital buttons
                if (wButtons & (1 << 0)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_UP;
                if (wButtons & (1 << 1)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
                if (wButtons & (1 << 2)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
                if (wButtons & (1 << 3)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
                if (wButtons & (1 << 4)) pad->wButtons |= XINPUT_GAMEPAD_START;
                if (wButtons & (1 << 5)) pad->wButtons |= XINPUT_GAMEPAD_BACK;
                if (wButtons & (1 << 6)) pad->wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
                if (wButtons & (1 << 7)) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
                if (wButtons & (1 << 8)) pad->wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
                if (wButtons & (1 << 9)) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
                if (wButtons & (1 << 12)) pad->wButtons |= XINPUT_GAMEPAD_A;
                if (wButtons & (1 << 13)) pad->wButtons |= XINPUT_GAMEPAD_B;
                if (wButtons & (1 << 14)) pad->wButtons |= XINPUT_GAMEPAD_X;
                if (wButtons & (1 << 15)) pad->wButtons |= XINPUT_GAMEPAD_Y;

                //Map the left and right triggers
                pad->bLeftTrigger = rdata[8];
                pad->bRightTrigger = rdata[9];

                //Map analog sticks
                pad->sThumbLX = rdata[11] << 8 | rdata[10];
                pad->sThumbLY = rdata[13] << 8 | rdata[12];
                pad->sThumbRX = rdata[15] << 8 | rdata[14];
                pad->sThumbRY = rdata[17] << 8 | rdata[16];

                xid_itf->new_pad_data = true;
            }
        }
        else if (xid_itf->type == XBOXONE)
        {
            if (rdata[0] == GIP_CMD_INPUT)
            {
                tu_memclr(pad, sizeof(xinput_gamepad_t));
                uint16_t wButtons = rdata[5] << 8 | rdata[4];

                //Map digital buttons
                if (wButtons & (1 << 8)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_UP;
                if (wButtons & (1 << 9)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
                if (wButtons & (1 << 10)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
                if (wButtons & (1 << 11)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
                if (wButtons & (1 << 2)) pad->wButtons |= XINPUT_GAMEPAD_START;
                if (wButtons & (1 << 3)) pad->wButtons |= XINPUT_GAMEPAD_BACK;
                if (wButtons & (1 << 14)) pad->wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
                if (wButtons & (1 << 15)) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
                if (wButtons & (1 << 12)) pad->wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
                if (wButtons & (1 << 13)) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
                if (wButtons & (1 << 4)) pad->wButtons |= XINPUT_GAMEPAD_A;
                if (wButtons & (1 << 5)) pad->wButtons |= XINPUT_GAMEPAD_B;
                if (wButtons & (1 << 6)) pad->wButtons |= XINPUT_GAMEPAD_X;
                if (wButtons & (1 << 7)) pad->wButtons |= XINPUT_GAMEPAD_Y;
                if (rdata[22] && 0x01) pad->wButtons   |= XINPUT_GAMEPAD_SHARE;

                //Map the left and right triggers
                pad->bLeftTrigger = (rdata[7] << 8 | rdata[6]) >> 2;
                pad->bRightTrigger = (rdata[9] << 8 | rdata[8]) >> 2;

                //Map analog sticks
                pad->sThumbLX = rdata[11] << 8 | rdata[10];
                pad->sThumbLY = rdata[13] << 8 | rdata[12];
                pad->sThumbRX = rdata[15] << 8 | rdata[14];
                pad->sThumbRY = rdata[17] << 8 | rdata[16];

                xid_itf->new_pad_data = true;
            }
            else if (rdata[0] == GIP_CMD_VIRTUAL_KEY)
            {
                if (rdata[4] == 0x01 && !(pad->wButtons & XINPUT_GAMEPAD_GUIDE)) {
                    xid_itf->new_pad_data = true;
                    pad->wButtons |= XINPUT_GAMEPAD_GUIDE;
                }
                else if (rdata[4] == 0x00 && (pad->wButtons & XINPUT_GAMEPAD_GUIDE)) {
                    xid_itf->new_pad_data = true;
                    pad->wButtons &= ~XINPUT_GAMEPAD_GUIDE;
                }
            }
            else if (rdata[0] == GIP_CMD_ANNOUNCE)
            {
                xboxone_init(xid_itf, dev_addr, instance);
            }
        }
        else if (xid_itf->type == XBOXOG)
        {
            if (rdata[1] == 0x14)
            {
                tu_memclr(pad, sizeof(xinput_gamepad_t));
                uint16_t wButtons = rdata[3] << 8 | rdata[2];

                //Map digital buttons
                if (wButtons & (1 << 0)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_UP;
                if (wButtons & (1 << 1)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
                if (wButtons & (1 << 2)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
                if (wButtons & (1 << 3)) pad->wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
                if (wButtons & (1 << 4)) pad->wButtons |= XINPUT_GAMEPAD_START;
                if (wButtons & (1 << 5)) pad->wButtons |= XINPUT_GAMEPAD_BACK;
                if (wButtons & (1 << 6)) pad->wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
                if (wButtons & (1 << 7)) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;

                if (rdata[4] > 0x20) pad->wButtons |= XINPUT_GAMEPAD_A;
                if (rdata[5] > 0x20) pad->wButtons |= XINPUT_GAMEPAD_B;
                if (rdata[6] > 0x20) pad->wButtons |= XINPUT_GAMEPAD_X;
                if (rdata[7] > 0x20) pad->wButtons |= XINPUT_GAMEPAD_Y;
                if (rdata[8] > 0x20) pad->wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
                if (rdata[9] > 0x20) pad->wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;

                //Map the left and right triggers
                pad->bLeftTrigger = rdata[10];
                pad->bRightTrigger = rdata[11];

                //Map analog sticks
                pad->sThumbLX = rdata[13] << 8 | rdata[12];
                pad->sThumbLY = rdata[15] << 8 | rdata[14];
                pad->sThumbRX = rdata[17] << 8 | rdata[16];
                pad->sThumbRY = rdata[19] << 8 | rdata[18];

                xid_itf->new_pad_data = true;
            }
        }
        if (xid_itf->new_pad_data)
        {
            tuh_xinput_report_received_cb(dev_addr, instance, xid_itf, sizeof(xinputh_interface_t));
            xid_itf->new_pad_data = false;
        } else {
            tuh_xinput_receive_report(dev_addr, instance);
        }
    }
    else
    {
        if (tuh_xinput_report_sent_cb)
        {
            tuh_xinput_report_sent_cb(dev_addr, instance, xid_itf->epout_buf, xferred_bytes);
        }
    }

    return true;
}

void xinputh_close(uint8_t dev_addr)
{
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX, );
    xinputh_device_t *xinput_dev = get_dev(dev_addr);

    for (uint8_t inst = 0; inst < xinput_dev->inst_count; inst++)
    {
        if (tuh_xinput_umount_cb)
        {
            tuh_xinput_umount_cb(dev_addr, inst);
        }
    }
    tu_memclr(xinput_dev, sizeof(xinputh_device_t));
}

#ifndef DRIVER_NAME
#if CFG_TUSB_DEBUG >= CFG_TUH_LOG_LEVEL
  #define DRIVER_NAME(_name)    .name = _name,
#else
  #define DRIVER_NAME(_name)
#endif
#endif

usbh_class_driver_t const usbh_xinput_driver =
{
    DRIVER_NAME("XINPUT")
    .init       = xinputh_init,
    .open       = xinputh_open,
    .set_config = xinputh_set_config,
    .xfer_cb    = xinputh_xfer_cb,
    .close      = xinputh_close
};

#endif