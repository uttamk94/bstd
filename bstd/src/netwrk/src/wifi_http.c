#include "wifi_http.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/net_event.h>

#include "loggers.h"

// Semaphores to synchronize connection and IP assignment
K_SEM_DEFINE(wifi_connected, 0, 1);
static struct net_mgmt_event_callback mgmt_cb;
static void wifi_mgmt_event_handler(
    struct net_mgmt_event_callback *cb,
    uint64_t mgmt_event,
    struct net_if *iface)
{
    ARG_UNUSED(cb);
    ARG_UNUSED(iface);

    printk("EVENT: 0x%llx\n", mgmt_event);

    if (mgmt_event & NET_EVENT_L4_CONNECTED) {

        printk("NETWORK READY\n");

        //k_sem_give(&wifi_connected);
    }

    if (mgmt_event & NET_EVENT_L4_DISCONNECTED) {

        printk("NETWORK DOWN\n");
    }
}

int start_wifi_connection(char *ssid, char *psk)
{
    struct net_if *iface = net_if_get_first_wifi();
    net_if_up(iface);

    net_mgmt_init_event_callback(&mgmt_cb,
                                wifi_mgmt_event_handler,

                                UINT64_MAX);
    net_mgmt_add_event_callback(&mgmt_cb);

    struct wifi_connect_req_params params = {
        .ssid           = ssid,
        .ssid_length    = strlen(ssid),
        .psk            = psk,
        .psk_length     = strlen(psk),
        .channel        = WIFI_CHANNEL_ANY,
        .security       = WIFI_SECURITY_TYPE_PSK,
    };

    log_i("Requesting connection to %s", ssid);

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params))) {
        log_i("Connection request failed to start");
        return -1;
    }
    return 0;
}

int connect_network(char *ssid, char *psk){
    return start_wifi_connection(ssid, psk);
}

int init_wifi_http() {
    return 0;
}

int start_wifi_http() {
    return 0;
}

int stop_wifi_http() {
    return 0;
}