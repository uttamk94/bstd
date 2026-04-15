
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <string.h>

#include "llog_svc.h"
#include "conn.h"
#include "loggers.h"

#define DEVICE_NAME "BSTD_DEV"
#define LOG_MAX_LEN 80

const static struct bt_gatt_attr *log_attr;

static char pending_log[LOG_MAX_LEN];
static atomic_t log_ready = ATOMIC_INIT(0);
static bool notify_enabled = false;

static struct bt_uuid_128 log_svc_uuid = BT_UUID_INIT_128(
    0x10,0x20,0x30,0x40,
    0x50,0x60,
    0x70,0x80,
    0x90,0xA0,
    0xB0,0xC0,0xD0,0xE0,0xF0,0x00
);

static struct bt_uuid_128 log_char_uuid = BT_UUID_INIT_128(
    0x11,0x22,0x33,0x44,
    0x55,0x66,
    0x77,0x88,
    0x99,0xAA,
    0xBB,0xCC,0xDD,0xEE,0xFF,0x00
);

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    log_i("[BLE] CCC changed: %u", value);
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

BT_GATT_SERVICE_DEFINE(log_svc,
    BT_GATT_PRIMARY_SERVICE(&log_svc_uuid),
    BT_GATT_CHARACTERISTIC(&log_char_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static void init_attr(void) {
    log_attr = &log_svc.attrs[1];
}

static inline void ble_log_send(const char *msg) {
    log_i("ble log send %s", msg);
    struct bt_conn *conn = get_conn();
    if (!conn) {
        log_e("[BLE] no conn");
        return;
    }

    if (!notify_enabled) {
        log_e("[BLE] no notify enabled");
        return;
    }

    int ret = bt_gatt_notify(conn, log_attr, msg, strlen(msg));
    log_i("[BLE] notify sent: %d", ret);
}

void ble_log(const char *msg, int len) {
    strncpy(pending_log, msg, LOG_MAX_LEN - 1);
    pending_log[LOG_MAX_LEN - 1] = '\0';

    atomic_set(&log_ready, 1);
    ble_log_send(pending_log);
    atomic_set(&log_ready, 0);
}

int init_llog_svc(void) {
    notify_enabled = false;
    return 0;
}

int start_llog_svc(void) {
    init_attr();
    return 0;
}