
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "adv.h"
#include "loggers.h"

#define DEVICE_NAME "BSTD_DEV"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Example Company ID (replace with real if you have one) */
#define COMPANY_ID_LSB              0xFF
#define COMPANY_ID_MSB              0xFF

#define PROTOCOL_VERSION            0x01
#define DEVICE_TYPE                 0xA1


static void adv_start(struct k_work *work);
K_WORK_DEFINE(adv_start_work, adv_start);

static uint8_t mfg_payload[16] = {
    COMPANY_ID_LSB, COMPANY_ID_MSB,
    PROTOCOL_VERSION,
    DEVICE_TYPE,
    1, 0,                           /* FW version 1.0 */
    0xDE, 0xAD, 0xBE, 0xEF,         /* Device ID */
    0, 0, 0, 0,                     /* Telemetry */
    0, 0                            /* Reserved */
};

/* Advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_payload, sizeof(mfg_payload)),
};

/* Scan response (optional) */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE,  DEVICE_NAME, DEVICE_NAME_LEN),
};

struct bt_le_adv_param adv_param = {
    .options = BT_LE_ADV_OPT_CONN,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .peer = NULL,
};

/* Connection callbacks (optional for extensibility) */
static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        log_i("BLE conn failed (err %u)", err);
    } else {
        log_i("BLE connected");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    log_i("BLE disconnected (reason %u)", reason);
    k_work_submit(&adv_start_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected      = connected,
    .disconnected   = disconnected,
};

static void adv_start(struct k_work *work) {
    int err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        log_i("Advertising failed to start (err %d)", err);
        return;
    }
    log_i("BLE advertising started");
}

int start_ble_adv(void) {
    k_work_submit(&adv_start_work);
    return 0;
}

void stop_ble_adv(void) {
    bt_le_adv_stop();
    log_i("BLE advertising stopped");
}

int init_adv(void) {
    log_i("Initializing BLE...");
    int err = bt_enable(NULL);
    if (err) {
        log_i("BLE init failed (err %d)", err);
        return err;
    }

    log_i("BLE initialized");
    return 0;
}