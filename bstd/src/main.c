/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <stdio.h>
#include "loggers.h"

#ifdef CONFIG_NVS_MGR_ENABLE
#include "nvs_mgr.h"
#endif
#ifdef CONFIG_DEV_SETT
#include "dev_sett.h"
#endif

#ifdef CONFIG_BLE_ENABLE
#include "ble.h"
#endif

#ifdef CONFIG_COMMU_ENABLE
#include "commu.h"
#endif

#ifdef CONFIG_FEATURE_ENABLE
#include "feature.h"
#endif

#ifdef CONFIG_SENSOR
#include "sensor.h"
#endif

#ifdef CONFIG_NETWORK_MOD
#include "netwrk.h"
#endif

#ifdef CONFIG_SHELL_MOD
#include "shell_main.h"
#endif

#define LOOKUP(mod) { .init_func = init_##mod, .start_func = start_##mod }
#define ARY_SZ(ary) (sizeof(ary) / sizeof(ary[0]))

typedef struct {
	int (*init_func)(void);
	int (*start_func)(void);
	int (*stop_func)(void);
} app_init_t;

typedef struct {
	unsigned char cmd;
	unsigned char len;
	void *data;
} mmsg_t;

K_MSGQ_DEFINE(main_msgq, 10, sizeof(mmsg_t), 4);

app_init_t look_up[] = {
#ifdef CONFIG_NVS_MGR_ENABLE
	{ .init_func = init_nvs_mgr, .start_func = start_nvs_mgr, .stop_func = stop_nvs_mgr },
#endif
#ifdef CONFIG_SENSOR
	{ .init_func = init_sensor, .start_func = start_sensor, .stop_func = stop_sensor },
#endif
#ifdef CONFIG_BLE_ENABLE
	{ .init_func = init_ble, .start_func = start_ble, .stop_func = stop_ble },
#endif
#ifdef CONFIG_COMMU_ENABLE
	{ .init_func = init_commu, .start_func = start_commu, .stop_func = stop_commu },
#endif
#ifdef CONFIG_DEV_SETT
	{ .init_func = init_dev_sett, .start_func = start_dev_sett, .stop_func = stop_dev_sett },
#endif
#ifdef CONFIG_FEATURE_ENABLE
	{ .init_func = init_feature, .start_func = start_feature, .stop_func = stop_feature },
#endif
#ifdef CONFIG_NETWORK_MOD
	{ .init_func = init_netwrk, .start_func = start_netwrk, .stop_func = stop_netwrk },
#endif
#ifdef CONFIG_SHELL_MOD
	{ .init_func = init_shell, .start_func = start_shell, .stop_func = stop_shell },
#endif
};

int push_mmsg(unsigned char cmd, unsigned char len, void *data) {
	mmsg_t msg = {
		.cmd	= cmd,
		.len 	= len,
		.data 	= data,
	};
	return k_msgq_put(&main_msgq, &msg, K_NO_WAIT);
}

void handle_mmsg(mmsg_t *msg) {
	log_i("cmd: %u, len: %u", msg->cmd, msg->len);
}

int main(void) {
	log_i("BSTD started!!");

	for (int i = 0; i < ARY_SZ(look_up); i++) {
		look_up[i].init_func();
	}

	for (int i = 0; i < ARY_SZ(look_up); i++) {
		look_up[i].start_func();
	}

	mmsg_t msg = {0, };
	while (1)  {
		if (!k_msgq_get(&main_msgq, &msg, K_FOREVER)) {
			handle_mmsg(&msg);
		}
	}

	/* Graceful shutdown (never reached in current design) */
	log_i("Shutting down modules...");
	for (int i = ARY_SZ(look_up); i > 0; i--) {
		look_up[i - 1].stop_func();
	}

	return 0;
	}
