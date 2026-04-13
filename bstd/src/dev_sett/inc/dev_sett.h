#pragma once



typedef enum {
    DS_CHG_EVNT,
    DS_BAT_LVL,
    DS_TMP_EVNT,
    DS_HL_EVNT,
    DS_MAX
} ds_cmd_t;

typedef struct {
    char chg;
    char level;
    char tmp;
    char hl;
} ds_data_t;

typedef struct {
    void (*chg_cb)(char chg);
    void (*level_cb)(char level);
    void (*tmp_cb)(char tmp);
    void (*hl_cb)(char hl);
} ds_listner_t;

int insert_dev_sett(ds_cmd_t cmd, unsigned int len, void *data);
ds_data_t get_ds_data();

int set_ds_listner(ds_listner_t *listner);

int init_dev_sett();
int start_dev_sett();