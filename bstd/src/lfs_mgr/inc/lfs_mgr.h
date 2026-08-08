#pragma once
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#define LFS_MGR_PATH_MAX 256

#ifdef __cplusplus
extern "C" {
#endif


int lfs_mgr_is_mounted(void);
struct fs_mount_t *lfs_mgr_get_mount(void);

int init_lfs_mgr(void);
int start_lfs_mgr(void);
int stop_lfs_mgr(void);
#ifdef __cplusplus
}
#endif