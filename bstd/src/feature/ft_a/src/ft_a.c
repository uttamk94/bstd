#include <stdio.h>
#include "ft_a.h"

#include "sensor.h"
#include "shandler.h"


void sens_data(sens_type_t type, unsigned int len, void *data) {
    printf("sens_data\n");
    printf("%s: %d, %u,\n", __func__, type, len);
}


sns_handler_t ft_a_handler = {
    .cid = CID_FA,
    .type = SENS_TYPE_SPO2,
    .sensor_data_cb = sens_data,
    .sensor_status = NULL,
};

int start_ft_a() {
    printf("start_ft_a\n");
    add_sensor(&ft_a_handler);
    return 0;
}


int init_ft_a() {
    printf("init_ft_a\n");
    return 0;
}