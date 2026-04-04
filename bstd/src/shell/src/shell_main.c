#include <zephyr/kernel.h>
#include <stdio.h>
#include "shell_main.h"


#include <zephyr/shell/shell.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <ctype.h>

LOG_MODULE_REGISTER(app);

int init_shell() {
    printf("init_shell\n");
    return 0;
}

int start_shell() {
    printf("start_shell\n");
    return 0;
}

static int cmd_test_start(const struct shell *sh, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Log test started");
	return 0;
}

static int cmd_test_stop(const struct shell *sh, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "Log test stopped");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_test,
	SHELL_CMD_ARG(start, NULL, "Start log test", cmd_test_start, 0, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop log test.", cmd_test_stop, 0, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(test, &sub_test, "test", NULL);

