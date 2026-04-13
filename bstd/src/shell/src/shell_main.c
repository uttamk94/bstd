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

LOG_MODULE_REGISTER(app);

int init_shell() {
    log_i("init_shell");
    return 0;
}

int start_shell() {
    log_i("start_shell");
    return 0;
}

typedef struct {
	int num;
	unsigned char params[28];
} param_t;


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
	shell_print(sh, "stop: %d, %u %u", params.num, params.params[0], params.params[1]);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_test,
	SHELL_CMD_ARG(start, NULL, "Start log test", cmd_test_start, 0, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop log test.", cmd_test_stop, 0, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(test, &sub_test, "test", NULL);

