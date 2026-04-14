#pragma once
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>

struct bt_conn *get_conn();
void set_conn(struct bt_conn *conn);