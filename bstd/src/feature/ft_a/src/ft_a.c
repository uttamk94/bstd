#include <stdio.h>
#include "ft_a.h"

#include "sensor.h"



void sens_data(sens_type_t type, unsigned int len, void *data) {
    printf("sens_data\n");
    printf("%s: %d, %u,\n", __func__, type, len);
}

int start_ft_a() {
    printf("start_ft_a\n");
    reg_sensor(SENS_TYPE_SPO2, sens_data);
    return 0;
}

int init_ft_a() {
    printf("init_ft_a\n");
    return 0;
}