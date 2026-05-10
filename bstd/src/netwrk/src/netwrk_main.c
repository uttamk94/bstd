

#include <zephyr/kernel.h>
#include "netwrk_main.h"
#include "wifi_http.h"
#include "loggers.h"
#include "netwrk_task.h"




void on_ntwrk_connect(void *data, unsigned char len) {
    log_i("start connection");
    connect_network();
}

void on_ntwrk_disconnect(void *data, unsigned char len) {
    log_i("start disconnection");
    //connect_network();
}


void on_ntwrk_msg_rcvd(void *data, unsigned char len) {
    log_i("ping url");
    //connect_network();
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
