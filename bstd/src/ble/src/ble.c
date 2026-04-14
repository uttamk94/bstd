#include "ble.h"
#include <stdio.h>
#include "loggers.h"

int init_ble() {
    log_i("init_ble");
    init_adv();
    init_data_svc();
    init_llog_svc();
    return 0;
}

int start_ble() {
    log_i("start_ble");
    start_ble_adv();
    start_llog_svc();
    return 0;
}