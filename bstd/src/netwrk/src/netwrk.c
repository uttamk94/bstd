#include "netwrk.h"
#include "loggers.h"

int init_netwrk(void) {
    log_i("Initializing network");
    init_netwrk_task();
    init_wifi_http();
    init_netwrk_main();
    return 0;
}

int start_netwrk(void) {
    log_i("Starting network");
    start_netwrk_task();
    start_wifi_http();
    start_netwrk_main();
    return 0;
}

int stop_netwrk(void) {
    log_i("Stopping network");
    stop_netwrk_main();
    stop_wifi_http();
    stop_netwrk_task();
    return 0;
}
