
#include "commu.h"
#include "loggers.h"

int init_commu() {
    init_data_commu();
    init_clnt_a();
    init_clnt_b();
    return 0;
}

int start_commu(void) {
    log_i("Starting communication");
    start_data_commu();
    start_clnt_a();
    start_clnt_b();
    return 0;
}

int stop_commu(void) {
    log_i("Stopping communication");
    stop_clnt_b();
    stop_clnt_a();
    stop_data_commu();
    return 0;
}