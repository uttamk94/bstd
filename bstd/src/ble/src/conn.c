#include "conn.h"

static struct bt_conn *current_conn = NULL;

struct bt_conn *get_conn() {
    return current_conn;
}

void set_conn(struct bt_conn *conn) {
    current_conn = conn;
}