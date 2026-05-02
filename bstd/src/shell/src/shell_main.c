#include <zephyr/kernel.h>
#include <stdio.h>
#include "shell_main.h"


#include <zephyr/shell/shell.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <ctype.h>
#include "loggers.h"
#include "nvs_mgr.h"

LOG_MODULE_REGISTER(app);

typedef struct {
	int num;
	unsigned char params[28];
} param_t;

int init_shell() {
    log_i("init_shell");
    return 0;
}

int start_shell() {
    log_i("start_shell");
    return 0;
}

static int parse_args(int argc, char **argv, param_t *params) {
	log_i("parse_args");
	params->num = argc - 1;
	for (int i = 0; i < params->num; i++) {
		params->params[i] = (unsigned char) atoi(argv[i + 1]);
	}
	return argc - 1;
}

static int cmd_test_start(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	shell_print(sh, "start: %d, %u %u", params.num, params.params[0], params.params[1]);
	return 0;
}

static int cmd_test_stop(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	shell_print(sh, "stop: %d, %u %u", params.num, params.params[0], params.params[0]);
	return 0;
}

static int test_nvs_write(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	shell_print(sh, "write: id: %d, len:%u", params.params[0], params.params[1]);
	ssize_t len = nvs_mgr_write(params.params[0], params.params[1], &params.params[2]);
	shell_print(sh, "len: %d", len);
	return 0;
}

static int test_nvs_read(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	shell_print(sh, "read: id:%u len:%u", params.params[0], params.params[1]);
	ssize_t len = nvs_mgr_read(params.params[0], params.params[1], &params.params[2]);
	shell_print(sh, "rlen: %d data:[0]%d,[1]%d,[2]%d,[3]%d[4]%d", len, params.params[2], params.params[3], params.params[4], params.params[5], params.params[6]);
	return 0;
}

#include "capa_msg.h"
#include "ble_task.h"
static int test_mmsg_cpa(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	struct {
		unsigned char clint;
		mob_capa_msg_t msg;
	} cpa_msg = {0, };
	cpa_msg.clint = params.params[0];
	cpa_msg.msg.header.type = params.params[1]; // request
	cpa_msg.msg.header.sz = 0; // variable
	cpa_msg.msg.header.msg_id = 0x00;
	insert_ble_msg(BLE_CMD_DATA, sizeof(cpa_msg), &cpa_msg);
	return 0;
}

static int test_mmsg_req(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	return 0;
}

static int test_mmsg_sync(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	return 0;
}

static int test_mmsg_wmsg(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	return 0;
}

static int test_mmsg_test(const struct shell *sh, size_t argc, char **argv) {
	param_t params = {0, };
	parse_args(argc, argv, &params);
	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(nvs_test,
	SHELL_CMD_ARG(write, NULL, "id len data ", test_nvs_write, 0, 0),
	SHELL_CMD_ARG(read, NULL, "id len", test_nvs_read, 0, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(mmsg_test,
	SHELL_CMD_ARG(cpa, 	NULL, "clnt, 0/1:req/res", test_mmsg_cpa, 	0, 0),
	SHELL_CMD_ARG(req, 	NULL, "clnt, 0/1:req/res", test_mmsg_req, 	0, 0),
	SHELL_CMD_ARG(sync, NULL, "clnt, 0/1:req/res", test_mmsg_sync, 	0, 0),
	SHELL_CMD_ARG(wmsg, NULL, "clnt, 0/1:req/res", test_mmsg_wmsg, 	0, 0),
	SHELL_CMD_ARG(test, NULL, "clnt, 0/1:req/res", test_mmsg_test, 	0, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_test,
	SHELL_CMD_ARG(start, NULL, "Start log test", cmd_test_start, 0, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop log test.", cmd_test_stop, 0, 0),
	SHELL_CMD_ARG(nvs, &nvs_test, "nvs.", NULL, 0, 0),
	SHELL_CMD_ARG(mmsg, &mmsg_test, "mmgs.", NULL, 0, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(test, &sub_test, "test", NULL);