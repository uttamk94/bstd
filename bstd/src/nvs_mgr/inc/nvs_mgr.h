#pragma once
#include <zephyr/kvss/nvs.h>

ssize_t nvs_mgr_write(unsigned short id, unsigned short len, void *data);
ssize_t nvs_mgr_read(unsigned short id, unsigned short len, void *data);

int init_nvs_mgr(void);
int start_nvs_mgr(void);