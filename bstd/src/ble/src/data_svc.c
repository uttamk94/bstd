#include "data_svc.h"
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <string.h>

#include "conn.h"
#include "loggers.h"

static uint32_t counter;

/* TX notify attribute pointer (IMPORTANT FIX) */
const static struct bt_gatt_attr *tx_attr;


static struct bt_uuid_128 custom_service_uuid = BT_UUID_INIT_128(
    0xAB, 0x90, 0x78, 0x56,
    0x34, 0x12,
    0x34, 0x12,
    0x78, 0x56,
    0x34, 0x12, 0x78, 0x56, 0x34, 0x12
);

/* TX Characteristic UUID: ...01 */
static struct bt_uuid_128 tx_char_uuid = BT_UUID_INIT_128(
    0x01, 0x00, 0x00, 0x00,
    0x34, 0x12,
    0x34, 0x12,
    0x78, 0x56,
    0x34, 0x12, 0x78, 0x56, 0x34, 0x12
);

/* RX Characteristic UUID: ...02 */
static struct bt_uuid_128 rx_char_uuid = BT_UUID_INIT_128(
    0x02, 0x00, 0x00, 0x00,
    0x34, 0x12,
    0x34, 0x12,
    0x78, 0x56,
    0x34, 0x12, 0x78, 0x56, 0x34, 0x12
);



/* ================= NOTIFY ================= */

void notify_value(void) {
    struct bt_conn *current_conn = get_conn();
    if (!current_conn) {
        return;
    }

    uint8_t data[4];
    data[0] = counter & 0xFF;
    data[1] = (counter >> 8) & 0xFF;
    data[2] = (counter >> 16) & 0xFF;
    data[3] = (counter >> 24) & 0xFF;

    bt_gatt_notify(current_conn,
                   tx_attr,
                   data,
                   sizeof(data));
    log_i("[BLE] notify sent: %u", counter);
}

/* ================= WRITE HANDLER ================= */

static ssize_t rx_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    uint8_t cmd[32];
    if (len > sizeof(cmd)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    memcpy(cmd, buf, len);

    log_i("[BLE] RX write received (%d bytes): ", len);

    for (int i = 0; i < len; i++) {
        log_i("%02X ", cmd[i]);
    }

    /* Example command handling */
    if (len > 0 && cmd[0] == 0xA1) {
        counter = 0;
        log_i("[BLE] counter reset");
    }

    return len;
}

/* ================= GATT TABLE ================= */
BT_GATT_SERVICE_DEFINE(custom_svc,
    BT_GATT_PRIMARY_SERVICE(&custom_service_uuid),

    BT_GATT_CHARACTERISTIC(&tx_char_uuid.uuid,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),

    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(&rx_char_uuid.uuid,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, rx_write, NULL),
);


static void init_tx_attr(void) {
    tx_attr = &custom_svc.attrs[1];
}

int init_data_svc(void) {
    init_tx_attr();
    return 0;
}

int start_data_svc(void) {
    return 0;
}