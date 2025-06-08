#include "usb/device/device_private.h"
#include "board_config.h"
#include "log/log.h"
#if UART_BRIDGE_ENABLED

#include <string.h>
#include <stdbool.h>
#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>
#include "usb/descriptors/cdc.h"

#define CDC_BAUDRATE            ((uint32_t)115200)
#define CDC_WORD_SIZE           ((uint8_t)8)
#define CDC_BUFFER_SIZE         (64U * 16U)

#define UART_BRIDGE_INST        __CONCAT(uart, UART_BRIDGE_UART_NUM)

typedef struct {
    uint8_t  buf[CDC_BUFFER_SIZE] __attribute__((aligned(4)));
    uint16_t head, tail;
} uart_ring_t;

typedef struct {
    bool                    alloc;
    bool                    configured;
    usb_cdc_line_coding_t   line_coding;
    uint16_t                line_state;
    bool                    dtr;
    bool                    rts;
    uint8_t                 ep_buf_out[CDC_EPSIZE_DATA_OUT] __attribute__((aligned(4)));
    uint8_t                 ep_buf_in[CDC_EPSIZE_DATA_IN] __attribute__((aligned(4)));
    uart_ring_t             uart_tx;
    uart_ring_t             uart_rx;
} uart_bridge_state_t;

static const cdc_desc_config_t UART_BRIDGE_DESC_CONFIG = {
    .config = {
        .bLength                = sizeof(usb_desc_config_t),
        .bDescriptorType        = USB_DTYPE_CONFIGURATION,
        .wTotalLength           = sizeof(cdc_desc_config_t),
        .bNumInterfaces         = CDC_ITF_NUM_TOTAL,
        .bConfigurationValue    = 1,
        .iConfiguration         = 0,
        .bmAttributes           = USB_ATTR_RESERVED | USB_ATTR_SELF_POWERED,
        .bMaxPower              = 50, // 100mA
    },
    .com_iad = {
        .bLength                = sizeof(usb_desc_iad_t),
        .bDescriptorType        = USB_DTYPE_IAD,
        .bFirstInterface        = CDC_ITF_NUM_CTRL,
        .bInterfaceCount        = 2,
        .bFunctionClass         = USB_CLASS_CDC,
        .bFunctionSubClass      = USB_SUBCLASS_CDC_ABSTRACT_CONTROL,
        .bFunctionProtocol      = CDC_PROTOCOL,
        .iFunction              = 0
    },
    .com_itf = {
        .bLength                = sizeof(usb_desc_itf_t),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = CDC_ITF_NUM_CTRL,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 1,
        .bInterfaceClass        = USB_CLASS_CDC,
        .bInterfaceSubClass     = USB_SUBCLASS_CDC_ABSTRACT_CONTROL,
        .bInterfaceProtocol     = CDC_PROTOCOL,
        .iInterface             = 0
    },
    .cdc_header = {
        .bFunctionLength        = sizeof(usb_cdc_desc_header_t),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_HEADER,
        .bcdCDC                 = 0x0110
    },
    .cdc_call_mgmt = {
        .bFunctionLength        = sizeof(usb_cdc_desc_call_mgmt_t),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_CALL_MGMT,
        .bmCapabilities         = USB_CDC_CALL_MGMT_CAP_CALL_MGMT,
        .bDataInterface         = CDC_ITF_NUM_DATA
    },
    .cdc_acm = {
        .bFunctionLength        = sizeof(usb_cdc_desc_acm_t),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_ACM,
        .bmCapabilities         = USB_CDC_ACM_CAP_LINE_CODING | USB_CDC_ACM_CAP_SEND_BREAK
    },
    .cdc_union = {
        .bFunctionLength        = sizeof(usb_cdc_desc_union_t),
        .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
        .bDescriptorSubType     = USB_DTYPE_CDC_UNION,
        .bMasterInterface0      = CDC_ITF_NUM_CTRL,
        .bSlaveInterface0       = CDC_ITF_NUM_DATA
    },
    .cdc_notif_ep = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC_EPADDR_NOTIF_IN,
        .bmAttributes           = USB_EP_TYPE_INTERRUPT,
        .wMaxPacketSize         = CDC_EPSIZE_NOTIF_IN,
        .bInterval              = 0xFF
    },
    .cdc_data_itf = {
        .bLength                = sizeof(usb_desc_itf_t),
        .bDescriptorType        = USB_DTYPE_INTERFACE,
        .bInterfaceNumber       = CDC_ITF_NUM_DATA,
        .bAlternateSetting      = 0,
        .bNumEndpoints          = 2,
        .bInterfaceClass        = USB_CLASS_CDC_DATA,
        .bInterfaceSubClass     = USB_SUBCLASS_NONE,
        .bInterfaceProtocol     = USB_PROTOCOL_NONE,
        .iInterface             = 0
    },
    .cdc_data_ep_out = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC_EPADDR_DATA_OUT,
        .bmAttributes           = USB_EP_TYPE_BULK,
        .wMaxPacketSize         = CDC_EPSIZE_DATA_OUT,
        .bInterval              = 1
    },
    .cdc_data_ep_in = {
        .bLength                = sizeof(usb_desc_endpoint_t),
        .bDescriptorType        = USB_DTYPE_ENDPOINT,
        .bEndpointAddress       = CDC_EPADDR_DATA_IN,
        .bmAttributes           = USB_EP_TYPE_BULK,
        .wMaxPacketSize         = CDC_EPSIZE_DATA_IN,
        .bInterval              = 1
    }
};

static uart_bridge_state_t uart_state = {0};

void uart_bridge_task(usbd_handle_t* handle);

static inline bool uart_ring_full(uart_ring_t* ring) {
    return ((ring->head + 1) % CDC_BUFFER_SIZE) == ring->tail;
}

static inline bool uart_ring_empty(uart_ring_t* ring) {
    return ring->head == ring->tail;
}

static inline uint16_t uart_ring_push(uart_ring_t* ring, const uint8_t* data, uint16_t len) {
    uint16_t count = 0;
    while (count < len) {
        if (((ring->head + 1) % CDC_BUFFER_SIZE) == ring->tail) {
            break;
        }
        ring->buf[ring->head] = data[count++];
        ring->head = (ring->head + 1) % CDC_BUFFER_SIZE;
    }
    return count;
}

static inline uint16_t uart_ring_pop(uart_ring_t* ring, uint8_t* data, uint16_t len) {
    uint16_t count = 0;
    while (count < len && ring->head != ring->tail) {
        data[count++] = ring->buf[ring->tail];
        ring->tail = (ring->tail + 1) % CDC_BUFFER_SIZE;
    }
    return count;
}

static void reset_line_state(uart_bridge_state_t* state) {
    state->line_coding.dwDTERate = CDC_BAUDRATE;
    state->line_coding.bCharFormat = USB_CDC_1_STOP_BITS;
    state->line_coding.bParityType = USB_CDC_NO_PARITY;
    state->line_coding.bDataBits = CDC_WORD_SIZE;
    state->line_state = 0U;
    state->dtr = false;
    state->rts = false;
}

static void reset_device_state(uart_bridge_state_t* state) {
    state->configured = false;
    reset_line_state(state);
    memset(&state->uart_rx, 0, sizeof(state->uart_rx));
    memset(&state->uart_tx, 0, sizeof(state->uart_rx));

    uart_set_fifo_enabled(UART_BRIDGE_INST, false);
    uint32_t err = uart_get_hw(UART_BRIDGE_INST)->rsr;
    uart_get_hw(UART_BRIDGE_INST)->rsr = err;
    uart_set_fifo_enabled(UART_BRIDGE_INST, true);
}

static void update_control_line_state(bool dtr, bool rts) {
    gpio_put(UART_BRIDGE_PIN_BOOT, dtr);
    gpio_put(UART_BRIDGE_PIN_RESET, rts);
}

static void uart_bridge_init_cb(usbd_handle_t* handle) {
    (void)handle;
}

static void uart_bridge_deinit_cb(usbd_handle_t* handle) {
    (void)handle;
    uart_state.configured = false;
}

static bool uart_bridge_get_desc_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    switch (USB_DESC_TYPE(req->wValue)) {
    case USB_DTYPE_DEVICE:
        return usbd_send_ctrl_resp(handle, &CDC_DESC_DEVICE, 
                                   sizeof(CDC_DESC_DEVICE), NULL);
    case USB_DTYPE_CONFIGURATION:
        return usbd_send_ctrl_resp(handle, &UART_BRIDGE_DESC_CONFIG, 
                                   sizeof(UART_BRIDGE_DESC_CONFIG), NULL);
    case USB_DTYPE_STRING:
        {
        const uint8_t idx = req->wValue & 0xFF;
        if (idx < ARRAY_SIZE(CDC_DESC_STR)) {
            return usbd_send_ctrl_resp(handle, CDC_DESC_STR[idx], 
                                       CDC_DESC_STR[idx]->bLength, NULL);
        } else if (idx == CDC_DESC_DEVICE.iSerialNumber) {
            const usb_desc_string_t* serial = usbd_get_desc_string_serial(handle);
            return usbd_send_ctrl_resp(handle, serial, serial->bLength, NULL);
        }
        }
        break;
    default:
        break;
    }
    return false;
}

static bool uart_bridge_ctrl_xfer_cb(usbd_handle_t* handle, const usb_ctrl_req_t* req) {
    if ((req->bmRequestType & (USB_REQ_TYPE_Msk | USB_REQ_RECIP_Msk)) !=
        (USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE)) {
        ogxm_logd("CDC: Invalid request type: %02X\n", req->bmRequestType);
        return false;
    }
    switch (req->bRequest) {
    case USB_REQ_CDC_GET_LINE_CODING:
        ogxm_logd("CDC: Get line coding\n");
        return usbd_send_ctrl_resp(handle, &uart_state.line_coding, 
                                   sizeof(uart_state.line_coding), NULL);
    case USB_REQ_CDC_SET_LINE_CODING:
        ogxm_logd("CDC: Set line coding\n");
        if (req->wLength != sizeof(uart_state.line_coding)) {
            ogxm_logd("CDC: Invalid line coding length: %d\n", req->wLength);
            return false;
        }
        if (memcmp(&uart_state.line_coding, req->data, sizeof(uart_state.line_coding)) == 0) {
            return true;
        }
        memcpy(&uart_state.line_coding, req->data, sizeof(uart_state.line_coding));
        uart_deinit(UART_BRIDGE_INST);
        uart_init(UART_BRIDGE_INST, uart_state.line_coding.dwDTERate);

        uart_parity_t partiy;
        switch (uart_state.line_coding.bParityType) {
        case USB_CDC_ODD_PARITY:
            partiy = UART_PARITY_ODD;
            break;
        case USB_CDC_EVEN_PARITY:
            partiy = UART_PARITY_EVEN;
            break;
        default:
            partiy = UART_PARITY_NONE;
            break;
        }
        uart_set_format(UART_BRIDGE_INST, uart_state.line_coding.bDataBits,
                        uart_state.line_coding.bCharFormat == USB_CDC_1_STOP_BITS ? 1 : 2,
                        partiy);
        uart_set_hw_flow(UART_BRIDGE_INST, uart_state.rts, uart_state.dtr);
        uart_set_fifo_enabled(UART_BRIDGE_INST, true);
        return true;
    case USB_REQ_CDC_SET_CONTROL_LINE_STATE:
        uart_state.line_state = req->wValue;
        uart_state.dtr = ((uart_state.line_state & USB_CDC_CONTROL_LINE_DTR) != 0);
        uart_state.rts = ((uart_state.line_state & USB_CDC_CONTROL_LINE_RTS) != 0);
        update_control_line_state(uart_state.dtr, uart_state.rts);
        ogxm_logd("CDC: Set control line state: DTR=%d, RTS=%d\n", 
            uart_state.dtr, uart_state.rts);
        return true;
    case USB_REQ_CDC_SEND_BREAK:
        ogxm_logd("CDC: Send break\n");
        if (req->wValue == 0) {
            uart_set_break(UART_BRIDGE_INST, true);
        } else {
            uart_set_break(UART_BRIDGE_INST, true);
            sleep_ms(req->wValue);
            uart_set_break(UART_BRIDGE_INST, false);
        }
        return true;
    default:
        ogxm_logd("CDC: Unknown req: %02X\n", req->bRequest);
        break;
    }
    return false;
}

static bool uart_bridge_set_config_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    return usbd_configure_all_eps(handle, &UART_BRIDGE_DESC_CONFIG);
}

static void uart_bridge_configured_cb(usbd_handle_t* handle, uint8_t config) {
    (void)config;
    reset_device_state(&uart_state);
    uart_state.configured = true;
}

static void uart_bridge_ep_xfer_cb(usbd_handle_t* handle, uint8_t epaddr) {
    switch (epaddr) {
    case CDC_EPADDR_DATA_IN:
        /* Data sent */
        if (usbd_ep_ready(handle, epaddr)) {
            uint16_t len = uart_ring_pop(&uart_state.uart_rx, uart_state.ep_buf_in, 
                                         CDC_EPSIZE_DATA_IN);
            if (len > 0) {
                usbd_ep_write(handle, epaddr, uart_state.ep_buf_in, len);
            }
        }
        break;
    case CDC_EPADDR_DATA_OUT:
        /* Data received */
        {
        int32_t len = usbd_ep_read(handle, epaddr, uart_state.ep_buf_out, 
                                   CDC_EPSIZE_DATA_OUT);
        while (len > 0) {
            len -= uart_ring_push(&uart_state.uart_tx, uart_state.ep_buf_out, len);
            if (len > 0) {
                uart_bridge_task(handle);
            }
        }
        }
        break;
    default:
        break;
    }
}

usbd_handle_t* uart_bridge_init(const usb_device_driver_cfg_t* cfg) {
    if (uart_state.alloc) {
        return NULL;
    }
    usbd_driver_t driver = {
        .init_cb = uart_bridge_init_cb,
        .deinit_cb = uart_bridge_deinit_cb,
        .get_desc_cb = uart_bridge_get_desc_cb,
        .ctrl_xfer_cb = uart_bridge_ctrl_xfer_cb,
        .set_config_cb = uart_bridge_set_config_cb,
        .configured_cb = uart_bridge_configured_cb,
        .ep_xfer_cb = uart_bridge_ep_xfer_cb,
    };
    usbd_handle_t* handle = usbd_init(cfg->usb.hw_type, &driver, CDC_EPSIZE_CTRL);
    if (handle != NULL) {
        uart_state.alloc = true;
        reset_line_state(&uart_state);

        gpio_init(UART_BRIDGE_PIN_RX);
        gpio_init(UART_BRIDGE_PIN_TX);
        gpio_init(UART_BRIDGE_PIN_BOOT);
        gpio_init(UART_BRIDGE_PIN_RESET);
        gpio_set_dir(UART_BRIDGE_PIN_BOOT, GPIO_OUT);
        gpio_set_dir(UART_BRIDGE_PIN_RESET, GPIO_OUT);
        gpio_put(UART_BRIDGE_PIN_BOOT, 1);
        gpio_put(UART_BRIDGE_PIN_RESET, 1);
        gpio_set_function(UART_BRIDGE_PIN_RX, GPIO_FUNC_UART);
        gpio_set_function(UART_BRIDGE_PIN_TX, GPIO_FUNC_UART);
        uart_init(UART_BRIDGE_INST, uart_state.line_coding.dwDTERate);
        uart_set_format(UART_BRIDGE_INST, uart_state.line_coding.bDataBits,
                        uart_state.line_coding.bCharFormat,
                        uart_state.line_coding.bParityType);
        uart_set_hw_flow(UART_BRIDGE_INST, uart_state.rts, uart_state.dtr);
        uart_set_fifo_enabled(UART_BRIDGE_INST, true);
    }
    return handle;
}

void uart_bridge_task(usbd_handle_t* handle) {
    if ((handle == NULL) || !uart_state.configured) {
        return;
    }
    while (uart_is_readable(UART_BRIDGE_INST) && !uart_ring_full(&uart_state.uart_rx)) {
        uint8_t byte = uart_getc(UART_BRIDGE_INST);
        uart_ring_push(&uart_state.uart_rx, &byte, 1);
    }
    if (usbd_ep_ready(handle, CDC_EPADDR_DATA_IN)) {
        uint16_t len = uart_ring_pop(&uart_state.uart_rx, uart_state.ep_buf_in, CDC_EPSIZE_DATA_IN);
        if (len > 0) {
            usbd_ep_write(handle, CDC_EPADDR_DATA_IN, uart_state.ep_buf_in, len);
        }
    }
    while (uart_is_writable(UART_BRIDGE_INST) && !uart_ring_empty(&uart_state.uart_tx)) {
        uint8_t byte;
        uart_ring_pop(&uart_state.uart_tx, &byte, 1);
        uart_putc(UART_BRIDGE_INST, byte);
    }
}

const usb_device_driver_t USBD_DRIVER_UART_BRIDGE = {
    .name = "UART Bridge",
    .init = uart_bridge_init,
    .task = uart_bridge_task,
    .set_audio = NULL,
    .set_pad = NULL,   
};

#else // !UART_BRIDGE_ENABLED

usbd_handle_t* uart_bridge_init(const usb_device_driver_cfg_t* cfg) {
    (void)cfg;
    ogxm_loge("UART Bridge driver is not enabled. Please enable UART_BRIDGE_ENABLED in board_config.h.");
    return NULL;
}

const usb_device_driver_t USBD_DRIVER_UART_BRIDGE = {
    .name = "UART Bridge",
    .init = uart_bridge_init,
    .task = NULL,
    .set_audio = NULL,
    .set_pad = NULL,   
};

#endif // UART_BRIDGE_ENABLED