#pragma once

#include <stdint.h>
#include "lwip/netif.h"
#include "gamepad/gamepad.h"
#include "gamepad/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBSERVER_MTU 1500U

typedef void (*webserver_wait_loop_cb_t)(void);

bool webserver_init(netif_linkoutput_fn linkoutput_cb, gamepad_rumble_cb_t rumble_cb);
void webserver_wait_ready(webserver_wait_loop_cb_t wait_loop_cb);
void webserver_start(void);
void webserver_task(void);
void webserver_set_frame(const uint8_t* frame, uint16_t len);
void webserver_get_mac_addr(uint8_t mac[6]);
void webserver_set_pad(const gamepad_pad_t* pad);

#ifdef __cplusplus
}
#endif