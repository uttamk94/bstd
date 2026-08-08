#include <zephyr/fs/littlefs.h>
#include <zephyr/fs/fs.h>
#include <string.h>
#include "lfs_mgr.h"
#include "loggers.h"

static struct fs_mount_t lfs_fs_mount;
static bool lfs_mounted = false;


int lfs_mgr_mount(void)
{
    int rc;

    if (lfs_mounted) {
        log_w("LittleFS already mounted");
        return 0;
    }

    if (!IS_ENABLED(CONFIG_LFS_MGR_ENABLE)) {
        log_e("LittleFS Manager not enabled");
        return -ENODEV;
    }

    if (!IS_ENABLED(CONFIG_LITTLEFS)) {
        log_e("LittleFS not enabled in system configuration");
        return -ENODEV;
    }

    memset(&lfs_fs_mount, 0, sizeof(lfs_fs_mount));
    lfs_fs_mount.type = FS_LITTLEFS;
    lfs_fs_mount.mnt_point = "/lfs:";

    if (strlen(CONFIG_LFS_MGR_STORAGE_DEV) > 0) {
        lfs_fs_mount.storage_dev = CONFIG_LFS_MGR_STORAGE_DEV;
    } else {
        lfs_fs_mount.storage_dev = "";
    }

    rc = fs_mount(&lfs_fs_mount);
    if (rc != 0) {
        log_e("Failed to mount LittleFS: %d", rc);
        return rc;
    }

    lfs_mounted = true;
        log_i("LittleFS mounted at %s", lfs_fs_mount.mnt_point);
    return 0;
}

int lfs_mgr_unmount(void)
{
    int rc;

    if (!lfs_mounted) {
        log_w("LittleFS not mounted");
        return 0;
    }

    rc = fs_unmount(&lfs_fs_mount);
    memset(&lfs_fs_mount, 0, sizeof(lfs_fs_mount));
    if (rc != 0) {
        log_e("Failed to unmount LittleFS: %d", rc);
        return rc;
    }

    lfs_mounted = false;
        log_i("LittleFS unmounted");
    return 0;
}

int lfs_mgr_is_mounted(void)
{
    return lfs_mounted ? 0 : -ENODEV;
}

struct fs_mount_t *lfs_mgr_get_mount(void)
{
    if (lfs_mounted) {
        return &lfs_fs_mount;
    }
    return NULL;
}

int init_lfs_mgr(void) {
    if (IS_ENABLED(CONFIG_LFS_MGR_ENABLE)) {
        log_i("LittleFS Manager initialized");
        return 0;
    }
    return -ENODEV;
}

int start_lfs_mgr(void) {
    lfs_mgr_mount();
    return 0;
}

int stop_lfs_mgr(void) {
    lfs_mgr_unmount();
    return 0;
}