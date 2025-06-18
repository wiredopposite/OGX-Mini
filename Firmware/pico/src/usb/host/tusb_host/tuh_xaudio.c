// #include <stdio.h>
// #include "tusb.h"
// #include "host/usbh.h"
// #include "host/usbh_pvt.h"
// #include "usb/host/host.h"
// // #include "class/hid/hid.h"

// #include "usb/host/host.h"
// #include "usb/descriptors/xid.h"
// #include "usb/descriptors/xinput.h"
// #include "usb/descriptors/xgip.h"
// #include "usb/host/tusb_host/hardware_ids.h"
// #include "usb/host/tusb_host/tuh_xaudio.h"

// #define USB_EP_IN               ((uint8_t)0x80)
// #define AUDIO_EPSIZE_MAX        0x100U
// #define AUDIO_CTRL_EPSIZE_MAX   0x40U

// typedef enum {
//     EP_IN = 0,
//     EP_OUT,
//     EP_COUNT
// } usbh_ep_type_t;

// typedef struct __attribute__((packed)) {
//     uint8_t  bLength;
//     uint8_t  bDescriptorType;
// } desc_header_t;

// typedef struct {
//     uint8_t  epaddr;
//     uint16_t size;
//     uint8_t  buf[AUDIO_EPSIZE_MAX] __attribute__((aligned(4)));
// } xa_ep_stream_t;

// typedef struct {
//     uint8_t  epaddr;
//     uint16_t size;
//     uint8_t  buf[AUDIO_CTRL_EPSIZE_MAX] __attribute__((aligned(4)));
// } xa_ep_ctrl_t;

// typedef struct {
//     bool                    mounted;
//     uint16_t                vid;
//     uint16_t                pid;
//     uint8_t                 daddr;
//     uint8_t                 itf_num;
//     usbh_type_t             type;
//     xa_ep_stream_t          ep_stream[EP_COUNT]; // IN and OUT endpoints
//     xa_ep_ctrl_t            ep_ctrl[EP_COUNT];   // Control endpoints
//     xa_send_complete_cb_t   send_stream_cb;
//     void*                   send_stream_ctx;
//     xa_send_complete_cb_t   send_ctrl_cb;
//     void*                   send_ctrl_ctx;
// } xa_interface_t;

// static xa_interface_t xa_interfaces[CFG_TUH_DEVICE_MAX] = {0};
// static size_t xa_dev_size = sizeof(xa_interfaces);

// static bool set_dev_type(xa_interface_t* itf, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
//     if (desc_itf->bInterfaceClass == 0x78 && 
//         desc_itf->bInterfaceSubClass == 0x00) {
//         itf->type = USBH_TYPE_XBLC;
//     } else if (desc_itf->bInterfaceSubClass == USB_SUBCLASS_XINPUT) {
//         switch (desc_itf->bInterfaceProtocol) {
//         case USB_PROTOCOL_XINPUT_AUDIO:
//             itf->type = USBH_TYPE_XINPUT_AUDIO;
//             break;
//         case USB_PROTOCOL_XINPUT_AUDIO_WL:
//             itf->type = USBH_TYPE_XINPUT_WL_AUDIO;
//         default:
//             break;
//         }
//     } else if (desc_itf->bInterfaceSubClass == 0x47 && 
//                desc_itf->bInterfaceProtocol == 0xD0) {
//         const uint8_t* desc_p = (const uint8_t*)desc_itf;
//         const uint8_t* desc_end = desc_p + max_len;
//         while (desc_p < desc_end) {
//             desc_p += tu_desc_len(desc_p);
//             if (tu_desc_type(desc_p) != TUSB_DESC_ENDPOINT) {
//                 continue;
//             }
//             const tusb_desc_endpoint_t* desc_ep = (const tusb_desc_endpoint_t*)desc_p;
//             if (desc_ep->bmAttributes.xfer == TUSB_XFER_ISOCHRONOUS) {
//                 itf->type = USBH_TYPE_XGIP_AUDIO;
//                 return true;  // Found an isochronous endpoint, this is audio!
//             }
//         }
//     }
//     return (itf->type != USBH_TYPE_NONE);
// }

// bool xaudio_init_cb(void) {
//     memset(xa_interfaces, 0, sizeof(xa_interfaces));
//     return true;
// }

// bool xaudio_deinit_cb(void) {
//     return xaudio_init_cb();
// }

// bool xaudio_open_cb(uint8_t rhport, uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
//     if ((daddr == 0) || (daddr > CFG_TUH_DEVICE_MAX)) {
//         return false;
//     }
//     xa_interface_t* itf = &xa_interfaces[daddr - 1];
//     if (itf->mounted) {
//         printf("Error: Interface already mounted\r\n");
//         return false;
//     }
//     itf->itf_num = desc_itf->bInterfaceNumber;
//     if (itf->vid == 0 && itf->pid == 0) {
//         tuh_vid_pid_get(daddr, &itf->vid, &itf->pid);
//     }
//     if (!set_dev_type(itf, desc_itf, max_len)) {
//         return false;
//     }
//     const uint8_t* desc_p = (const uint8_t*)desc_itf;
//     const uint8_t* desc_end = desc_p + max_len;
//     uint8_t eps = 0;
//     bool opened = false;

//     while ((desc_p < desc_end) && (eps < desc_itf->bNumEndpoints)) {
//         desc_p += tu_desc_len(desc_p);
//         if (desc_p >= desc_end) {
//             break;
//         }
//         const desc_header_t* desc_hdr = (const desc_header_t*)desc_p;
//         if ((desc_hdr->bLength == 0) || 
//             (desc_hdr->bDescriptorType == TUSB_DESC_INTERFACE)) {
//             break;
//         }
//         if (desc_hdr->bDescriptorType != TUSB_DESC_ENDPOINT) {
//             continue;
//         }
//         eps++;

//         const tusb_desc_endpoint_t* desc_ep = (const tusb_desc_endpoint_t*)desc_p;
//         TU_VERIFY(tuh_edpt_open(daddr, desc_ep));

//         if (desc_ep->bEndpointAddress & USB_EP_IN) {
//             if (itf->ep_stream[EP_IN].epaddr == 0) {
//                 itf->ep_stream[EP_IN].epaddr = desc_ep->bEndpointAddress;
//                 itf->ep_stream[EP_IN].size = tu_edpt_packet_size(desc_ep);
//             } else if (itf->ep_ctrl[EP_IN].epaddr == 0) {
//                 itf->ep_ctrl[EP_IN].epaddr = desc_ep->bEndpointAddress;
//                 itf->ep_ctrl[EP_IN].size = tu_edpt_packet_size(desc_ep);
//             } else {
//                 printf("Error: Too many IN endpoints\r\n");
//                 continue;
//             }
//         } else {
//             if (itf->ep_stream[EP_OUT].epaddr == 0) {
//                 itf->ep_stream[EP_OUT].epaddr = desc_ep->bEndpointAddress;
//                 itf->ep_stream[EP_OUT].size = tu_edpt_packet_size(desc_ep);
//             } else if (itf->ep_ctrl[EP_OUT].epaddr == 0) {
//                 itf->ep_ctrl[EP_OUT].epaddr = desc_ep->bEndpointAddress;
//                 itf->ep_ctrl[EP_OUT].size = tu_edpt_packet_size(desc_ep);
//             } else {
//                 printf("Error: Too many IN endpoints\r\n");
//                 continue;
//             }
//         }
//         tuh_xaudio_init_cb(itf->type, daddr, itf->itf_num);
//         itf->mounted = true;
//         opened = true;
//     }
//     return opened;
// }

// bool xaudio_set_config_cb(uint8_t daddr, uint8_t itf_num) {
//     xa_interface_t* itf = &xa_interfaces[daddr - 1];
//     tuh_xaudio_mounted_cb(daddr, itf_num);
//     return true;
// }

// bool xaudio_ep_xfer_cb(uint8_t daddr, uint8_t epaddr, xfer_result_t result, uint32_t len) {
//     xa_interface_t* itf = &xa_interfaces[daddr - 1];
//     if (epaddr == itf->ep_stream[EP_IN].epaddr) {
//         if (result == XFER_RESULT_SUCCESS) {
//             tuh_xaudio_data_received_cb(daddr, itf->itf_num, itf->ep_stream[EP_IN].buf, len);
//         } else {
//             tuh_xaudio_receive_data(daddr, itf->itf_num);
//         }
//     } else if (epaddr == itf->ep_ctrl[EP_IN].epaddr) {
//         if (result == XFER_RESULT_SUCCESS) {
//             tuh_xaudio_data_ctrl_received_cb(daddr, itf->itf_num, itf->ep_ctrl[EP_IN].buf, len);
//         } else {
//             tuh_xaudio_receive_data_ctrl(daddr, itf->itf_num);
//         }
//     } else if (epaddr == itf->ep_stream[EP_OUT].epaddr) {
//         if (itf->send_stream_cb != NULL) {
//             xa_send_complete_cb_t callback = itf->send_stream_cb;
//             void* ctx = itf->send_stream_ctx;
//             itf->send_stream_cb = NULL;
//             itf->send_stream_ctx = NULL;
//             callback(daddr, itf->itf_num, itf->ep_stream[EP_OUT].buf, len, 
//                      (result == XFER_RESULT_SUCCESS), ctx);
//         }
//     } else if (epaddr == itf->ep_ctrl[EP_OUT].epaddr) {
//         if (itf->send_ctrl_cb != NULL) {
//             xa_send_complete_cb_t callback = itf->send_ctrl_cb;
//             void* ctx = itf->send_ctrl_ctx;
//             itf->send_ctrl_cb = NULL;
//             itf->send_ctrl_ctx = NULL;
//             callback(daddr, itf->itf_num, itf->ep_ctrl[EP_OUT].buf, len, 
//                      (result == XFER_RESULT_SUCCESS), ctx);
//         }
//     }
//     return true;
// }

// void xaudio_close_cb(uint8_t daddr) {
//     xa_interface_t* itf = &xa_interfaces[daddr - 1];
//     tuh_xaudio_unmounted_cb(daddr, itf->itf_num);
//     memset(itf, 0, sizeof(xa_interface_t));
// }