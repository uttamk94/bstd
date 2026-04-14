#include "ble.h"
#include <stdio.h>
#include "loggers.h"

#include "adv.h"
#include "data_svc.h"
#include "conn.h"

int init_ble() {
    log_i("init_ble");
    init_adv();
    init_data_svc();
    return 0;
}

int start_ble() {
    log_i("start_ble");
    start_ble_adv();
    return 0;
}