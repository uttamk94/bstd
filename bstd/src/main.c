/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <stdio.h>

#ifdef CONFIG_DEV_SETT
#include "dev_sett.h"
#endif

#ifdef CONFIG_BLE_ENABLE
#include "ble.h"
#endif
#ifdef CONFIG_FEATURE_ENABLE
#include "feature.h"
#endif

#ifdef CONFIG_SENSOR
#include "sensor.h"
#endif

#ifdef CONFIG_SHELL_MOD
#include "shell_main.h"
#endif

#define LOOKUP(mod) { .init_func = init_##mod, .start_func = start_##mod }
#define ARY_SZ(ary) (sizeof(ary) / sizeof(ary[0]))

typedef struct {
	int (*init_func)(void);
	int (*start_func)(void);
} app_init_t;

app_init_t look_up[] = {
#ifdef CONFIG_SENSOR
	LOOKUP(sensor),
#endif
#ifdef CONFIG_BLE_ENABLE
	LOOKUP(ble),
#endif
#ifdef CONFIG_DEV_SETT
	LOOKUP(dev_sett),
#endif
#ifdef CONFIG_DEV_SETT
	LOOKUP(feature),
#endif
#ifdef CONFIG_SHELL_MOD
	LOOKUP(shell),
#endif
};

int main(void) {
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	for (int i = 0; i < ARY_SZ(look_up); i++)
		look_up[i].init_func();
	for (int i = 0; i < ARY_SZ(look_up); i++)
		look_up[i].start_func();

	int counter = 0;
	while (1)  {
		printf("%s: %d\n", __func__, ++counter);
		counter = counter % 99999;
		k_msleep(1000);
	}
	return 0;
}
