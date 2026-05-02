#include <stdio.h>
#include "ft_a.h"

#include "sensor.h"
#include "shandler.h"
#include "loggers.h"

#include "ble.h"

unsigned int count = 0;
void sens_data(unsigned char type, unsigned int len, void *data) {
    log_i("sens_data");
    log_i("%u, %u,", type, len);
    count++;
    //ble_log((char *)&count, sizeof(count));
}


sns_handler_t ft_a_handler = {
    .cid = CID_FA,
    .type = SENS_TYPE_SPO2,
    .sensor_data_cb = sens_data,
    .sensor_status = NULL,
};

int start_ft_a() {
    log_i("start_ft_a");
    add_sensor(&ft_a_handler);
    return 0;
}


int init_ft_a() {
    log_i("init_ft_a");
    return 0;
}