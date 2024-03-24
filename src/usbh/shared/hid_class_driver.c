#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/hid/hid_host.h"
#include "usbh/shared/hid_class_driver.h"

usbh_class_driver_t const usbh_hid_driver =
{
    .init       = hidh_init,
    .open       = hidh_open,
    .set_config = hidh_set_config,
    .xfer_cb    = hidh_xfer_cb,
    .close      = hidh_close
};