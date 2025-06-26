#include <pico/sync.h>
#include <pico/time.h>
#include <pico/unique_id.h>
#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/apps/httpd.h"

#include "log/log.h"
#include "led/led.h"
#include "shared.h"
#include "webserver/networking/dhserver.h"
#include "webserver/networking/dnserver.h"
#include "webserver/webserver.h"

typedef struct {
    bool inited;
    // uint16_t mtu;
    netif_linkoutput_fn linkoutput_cb;
    gamepad_rumble_cb_t rumble_cb;
    gamepad_pad_t pad;
    struct netif netif_data;
    uint8_t mac_address[6];
} webserver_state_t;

static const ip_addr_t IP_ADDR  = IPADDR4_INIT_BYTES(192, 168, 7, 1);
static const ip_addr_t NETMASK = IPADDR4_INIT_BYTES(255, 255, 255, 0);
static const ip_addr_t GATEWAY = IPADDR4_INIT_BYTES(0, 0, 0, 0);

/* database IP addresses that can be offered to the host; this must be in RAM to store assigned MAC addresses */
static dhcp_entry_t ENTRIES[] = {
    /* mac ip address                          lease time */
    { {0}, IPADDR4_INIT_BYTES(192, 168, 7, 2), 24 * 60 * 60 },
    { {0}, IPADDR4_INIT_BYTES(192, 168, 7, 3), 24 * 60 * 60 },
    { {0}, IPADDR4_INIT_BYTES(192, 168, 7, 4), 24 * 60 * 60 },
};

static const dhcp_config_t DHCP_CONFIG = {
    .router = IPADDR4_INIT_BYTES(0, 0, 0, 0),   /* router address (if any) */
    .port = 67,                                 /* listen port */
    .dns = IPADDR4_INIT_BYTES(192, 168, 7, 1),  /* dns server (if any) */
    "",                                         /* dns suffix */
    LWIP_ARRAYSIZE(ENTRIES),                    /* num entry */
    ENTRIES                                     /* entries */
};

static webserver_state_t webserver_state = {0};

// let our webserver do some dynamic handling
static const char *cgi_toggle_led(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    static bool led_state = false;
    led_state = !led_state;
    led_set_on(led_state);
    return "/index.html";
}

static const char *cgi_reset_usb_boot(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    // reset_usb_boot(0, 0);
    pico_full_reboot();
    return "/index.html";
}

static const tCGI CGI_HANDLERS[] = {
    {
        "/toggle_led",
        cgi_toggle_led
    },
    {
        "/reset_usb_boot",
        cgi_reset_usb_boot
    }
};

static err_t netif_init_cb(struct netif *netif) {
    if (netif == NULL) {
        ogxm_loge("WEBSERVER: Netif pointer cannot be NULL");
        return ERR_ARG;
    }
    netif->mtu = WEBSERVER_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->linkoutput = webserver_state.linkoutput_cb;
    netif->output = etharp_output;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    return ERR_OK;
}

bool dns_query_cb(const char *name, ip4_addr_t *addr) {
    ogxm_logv("WEBSERVER: DNS query for %s\n", name);
    // if (0 == strcmp(name, "wizio.pico")) {
        *addr = IP_ADDR;
        return true;
    // }
    // return false;
}

// Public API

bool webserver_init(netif_linkoutput_fn linkoutput_cb, gamepad_rumble_cb_t rumble_cb) {
    if (webserver_state.inited) {
        ogxm_logd("WEBSERVER: Already initialized\n");
        return true;
    }
    if (linkoutput_cb == NULL) {
        ogxm_loge("WEBSERVER: Link output callback cannot be NULL\n");
        return false;
    }
    webserver_state.inited = true;
    webserver_state.linkoutput_cb = linkoutput_cb;

    lwip_init();
    struct netif *netif = &webserver_state.netif_data;
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);

    webserver_state.mac_address[0] = 0x02;
    memcpy(webserver_state.mac_address + 1, board_id.id, sizeof(webserver_state.mac_address) - 1);
    memcpy(netif->hwaddr, webserver_state.mac_address, sizeof(webserver_state.mac_address));

    netif = netif_add(netif, &IP_ADDR, &NETMASK, &GATEWAY, NULL, netif_init_cb, ip_input);
    netif_set_default(netif);
    return true;
}

void webserver_wait_ready(webserver_wait_loop_cb_t wait_loop_cb) {
    while (!netif_is_up(&webserver_state.netif_data)) {
        if (wait_loop_cb != NULL) {
            wait_loop_cb();
        }
    }
    ogxm_logd("WEBSERVER: Network interface initialized\n");
    while (dhserv_init(&DHCP_CONFIG) != ERR_OK) {
        if (wait_loop_cb != NULL) {
            wait_loop_cb();
        }
    }
    ogxm_logd("WEBSERVER: DHCP server initialized\n");
    while (dnserv_init(&IP_ADDR, 53, dns_query_cb) != ERR_OK) {
        if (wait_loop_cb != NULL) {
            wait_loop_cb();
        }
    }
    ogxm_logd("WEBSERVER: DNS server initialized\n");
}

void webserver_start(void) {
    httpd_init();
    http_set_cgi_handlers(CGI_HANDLERS, LWIP_ARRAYSIZE(CGI_HANDLERS));
    ogxm_logi("WEBSERVER: Starting with MTU %d, MAC: %02x:%02x:%02x:%02x:%02x:%02x\n,", 
        WEBSERVER_MTU, 
        webserver_state.mac_address[0],
        webserver_state.mac_address[1],
        webserver_state.mac_address[2],
        webserver_state.mac_address[3],
        webserver_state.mac_address[4],
        webserver_state.mac_address[5]);
}

void webserver_task(void) {
    if (!webserver_state.inited) {
        return;
    }
    sys_check_timeouts();
}

void webserver_set_frame(const uint8_t* frame, uint16_t len) {
    struct pbuf* p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (p == NULL) {
        ogxm_loge("WEBSERVER: Failed to allocate pbuf for received data\n");
        return;
    }
    memcpy(p->payload, frame, len);
    if (ethernet_input(p, &webserver_state.netif_data) != ERR_OK) {
        pbuf_free(p);
    }
}

void webserver_get_mac_addr(uint8_t mac[6]) {
    if (mac == NULL) {
        ogxm_loge("WEBSERVER: MAC address pointer cannot be NULL");
        return;
    }
    memcpy(mac, webserver_state.mac_address, sizeof(webserver_state.mac_address));
}