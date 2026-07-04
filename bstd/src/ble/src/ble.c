#include "ble.h"
#include <stdio.h>
#include "loggers.h"

int init_ble(void) {
    log_i("Initializing BLE");
    init_adv();
    init_ble_task();
    init_data_svc();
    init_llog_svc();
    return 0;
}

int start_ble(void) {
    log_i("Starting BLE");
    start_ble_adv();
    start_ble_task();
    start_llog_svc();
    return 0;
}

int stop_ble(void) {
    log_i("Stopping BLE");
    stop_llog_svc();
    stop_data_svc();
    stop_ble_task();
    stop_ble_adv();
    log_i("BLE stopped");
    return 0;
}
