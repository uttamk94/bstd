#include "netwrk.h"
#include "loggers.h"

int init_netwrk() {
    log_i("begin");
    init_netwrk_task();
    init_wifi_http();
    init_netwrk_main();
    return 0;
}

int start_netwrk() {
    log_i("begin");
    start_netwrk_task();
    start_wifi_http();
    start_netwrk_main();
    return 0;
}