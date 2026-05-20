

#include <zephyr/kernel.h>
#include "netwrk_main.h"
#include "wifi_http.h"
#include "loggers.h"
#include "netwrk_task.h"
#include "netw_http_mgr.h"

static char rx_buf[1024];

int create_http_req(void) {
    netw_http_rsp_t rsp;
    log_i("http start");
    int ret = netw_http_get(
        "94.130.142.35",
        80,
        "api.open-meteo.com",
        "/v1/forecast?latitude=52.52&longitude=13.41&past_days=10&hourly=temperature_2m,relative_humidity_2m,wind_speed_10m",
        rx_buf,
        sizeof(rx_buf),
        &rsp);

    if (ret) {
        log_e("http failed %d", ret);
        return 0;
    }

    log_i("STATUS: %d", rsp.status);
    log_i("BODY: \n%d: %s", rsp.body_len, rsp.body);
    return 0;
}

void on_ntwrk_connect(void *data, unsigned char len) {
    log_i("start connection");
    unsigned char *buf = (unsigned char *) data;
    char ssid[32] = {0, };
    char psk[32] = {0, };
    int indx = 0;
    while (indx < len) {
        switch (buf[indx++])
        {
        case 0x01:{
            int len = 0;
            memcpy(&len, buf + indx, sizeof(len));
            indx += sizeof(len);
            memcpy(ssid, buf + indx, len);
            indx += len;
        } break;
        
        case 0x02:{
            int len = 0;
            memcpy(&len, buf + indx, sizeof(len));
            indx += sizeof(len);
            memcpy(psk, buf + indx, len);
            indx += len;
        } break;

        default:
            break;
        }
    }
    connect_network(ssid, psk);
}

void on_ntwrk_disconnect(void *data, unsigned char len) {
    log_i("start disconnection");
    //connect_network();
}

void on_ntwrk_msg_rcvd(void *data, unsigned char len) {
    log_i("ping url");
    create_http_req();
}

static ntwrk_msg_listner_t lstnr = {
    .lid = LID_MAIN,
    .count = 3,
    .cbs = {
        on_ntwrk_connect,
        on_ntwrk_disconnect,
        on_ntwrk_msg_rcvd
    }
};

int init_netwrk_main() {
    log_i("begin");
    return 0;
}

int start_netwrk_main() {
    log_i("begin");
    set_netwrk_listner(&lstnr);
    return 0;
}
