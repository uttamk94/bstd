
#include "nvs_mgr.h"
#include "loggers.h"

#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/nvs.h>

static struct nvs_fs nvs;

int nvs_init(void) {
    const struct flash_area *fa;
    if (flash_area_open(FIXED_PARTITION_ID(storage_partition), &fa))
        return -1;
    nvs.flash_device = fa->fa_dev;
    nvs.offset = fa->fa_off;
    nvs.sector_size = 4096;
    nvs.sector_count = fa->fa_size / 4096;
    return nvs_mount(&nvs);
}

ssize_t nvs_mgr_write(unsigned short id, unsigned short len, void *data) { 
    return nvs_write(&nvs, id, data, len);
}

ssize_t nvs_mgr_read(unsigned short id, unsigned short len, void *data) { 
    return nvs_read(&nvs, id, data, len);
 }

int init_nvs_mgr(void) { 
    log_i("Initializing NVS manager");
    int ret = nvs_init();
    if (ret != 0) {
        log_e("NVS mount failed: %d", ret);
        return ret;
    }
    return 0;
}

int start_nvs_mgr(void) { 
    log_i("NVS manager started");
    return 0; 
}

int stop_nvs_mgr(void) {
    log_i("Stopping NVS manager");
    /* Note: NVS unmount not available in current Zephyr version */
    return 0;
}
