#include "wifi_http.h"

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/net_event.h>

#include "loggers.h"

#define SSID "Disconnet"
#define PSK "qwerfdsa"

// Semaphores to synchronize connection and IP assignment
K_SEM_DEFINE(wifi_connected, 0, 1);
K_SEM_DEFINE(ipv4_address_obtained, 0, 1);

static struct net_mgmt_event_callback mgmt_cb;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                    uint64_t mgmt_event,
                                    struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        if (status->status == 0) {
            log_i("Wi-Fi connected successfully");
            k_sem_give(&wifi_connected);
        } else {
            log_i("Wi-Fi connection failed: %d", status->status);
        }
    }
    
    if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        log_i("Disconnect Reason: %d", status->status);
    }

    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        log_i("IPv4 address obtained!");
        k_sem_give(&ipv4_address_obtained);
    }
}

int start_wifi_connection(void)
{
    struct net_if *iface = net_if_get_default();
    
    // 1. Initialize Management Callbacks
    net_mgmt_init_event_callback(&mgmt_cb, wifi_mgmt_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT 
                                | NET_EVENT_IPV4_ADDR_ADD
                                | NET_EVENT_IPV6_ADDR_ADD
                                | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&mgmt_cb);

    // 2. Setup Connection Parameters (Short names for Zephyr 3.x+)
    struct wifi_connect_req_params params = {
        .ssid           = SSID,
        .ssid_length    = strlen(SSID),
        .psk            = PSK,
        .psk_length     = strlen(PSK),
        .channel        = WIFI_CHANNEL_ANY,
        .security       = WIFI_SECURITY_TYPE_PSK,
    };

    log_i("Requesting connection to %s", SSID);

    // 3. Request Connection
    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params))) {
        log_i("Connection request failed to start");
        return -1;
    }

    // 4. Wait for Layers to finish
    if (k_sem_take(&wifi_connected, K_SECONDS(100)) != 0) {
        log_i("Wi-Fi connection timed out");
        return -ETIMEDOUT;
    }

    if (k_sem_take(&ipv4_address_obtained, K_SECONDS(60)) != 0) {
        log_i("Failed to obtain IP address (DHCP failure)");
        return -ETIMEDOUT;
    }

    return 0;
}

int connect_network(void) {
    return start_wifi_connection();
}

int init_wifi_http() {
    return 0;
}

int start_wifi_http() {
    return 0;
}