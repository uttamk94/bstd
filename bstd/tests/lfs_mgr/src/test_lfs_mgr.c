#include <zephyr/ztest.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "lfs_mgr.h"

/* Test file paths */
#define TEST_FILE_PATH "/lfs:/test_file.txt"
#define TEST_FILE_PATH2 "/lfs:/test_file2.txt"
#define TEST_DIR_PATH "/lfs:/test_dir"
#define TEST_CONTENT "Hello LittleFS!"
#define TEST_CONTENT_LEN 16

/**
 * @brief Test lfs_mgr_is_mounted when not mounted
 */
ZTEST(test_lfs_mgr, test_is_mounted_when_not_mounted)
{
    int rc;

    /* Initially should not be mounted */
    rc = lfs_mgr_is_mounted();
    zassert_equal(rc, -ENODEV, "lfs_mgr should not be mounted initially");
}

/**
 * @brief Test lfs_mgr_get_mount when not mounted
 */
ZTEST(test_lfs_mgr, test_get_mount_when_not_mounted)
{
    struct fs_mount_t *mount;

    mount = lfs_mgr_get_mount();
    zassert_is_null(mount, "mount should be NULL when not mounted");
}

/**
 * @brief Test init_lfs_mgr when feature is enabled
 */
ZTEST(test_lfs_mgr, test_init_lfs_mgr_enabled)
{
    int rc;

    rc = init_lfs_mgr();
    zassert_equal(rc, 0, "init_lfs_mgr should return 0 when enabled");
}

/**
 * @brief Test stop_lfs_mgr when not mounted
 */
ZTEST(test_lfs_mgr, test_stop_when_not_mounted)
{
    int rc;

    /* Should handle gracefully when not mounted */
    rc = stop_lfs_mgr();
    zassert_equal(rc, 0, "stop_lfs_mgr should return 0 even when not mounted");
}

/**
 * @brief Test mount/unmount cycle
 */
ZTEST(test_lfs_mgr, test_mount_unmount_cycle)
{
    int rc;
    struct fs_mount_t *mount;

    /* Start the manager (mounts littlefs) */
    rc = start_lfs_mgr();
    zassert_equal(rc, 0, "start_lfs_mgr should return 0");

    /* Should be mounted now */
    rc = lfs_mgr_is_mounted();
    zassert_equal(rc, 0, "lfs_mgr should be mounted after start");

    /* Get mount point */
    mount = lfs_mgr_get_mount();
    zassert_not_null(mount, "mount should not be NULL when mounted");
    zassert_equal(mount->type, FS_LITTLEFS, "Mount type should be LITTLEFS");
    zassert_not_null(mount->mnt_point, "Mount point should not be NULL");

    /* Verify mount point path */
    zassert_true(strlen(mount->mnt_point) > 0,
                 "Mount point path should not be empty");

    /* Stop the manager (unmounts littlefs) */
    rc = stop_lfs_mgr();
    zassert_equal(rc, 0, "stop_lfs_mgr should return 0");

    /* Should be unmounted now */
    rc = lfs_mgr_is_mounted();
    zassert_equal(rc, -ENODEV, "lfs_mgr should not be mounted after stop");

    /* Mount should be NULL again */
    mount = lfs_mgr_get_mount();
    zassert_is_null(mount, "mount should be NULL when unmounted");
}

/**
 * @brief Test multiple mount attempts
 */
ZTEST(test_lfs_mgr, test_multiple_mount_attempts)
{
    int rc;
    struct fs_mount_t *mount;

    /* First mount */
    rc = start_lfs_mgr();
    zassert_equal(rc, 0, "First mount should succeed");

    /* Get mount info */
    mount = lfs_mgr_get_mount();
    zassert_not_null(mount, "mount should be valid after first mount");

    /* Second mount attempt should be safe */
    rc = start_lfs_mgr();
    zassert_equal(rc, 0, "Second mount attempt should be safe");

    /* Should still be mounted */
    mount = lfs_mgr_get_mount();
    zassert_not_null(mount, "mount should still be valid");

    /* Cleanup */
    rc = stop_lfs_mgr();
    zassert_equal(rc, 0, "stop_lfs_mgr should return 0");

    rc = lfs_mgr_is_mounted();
    zassert_equal(rc, -ENODEV, "lfs_mgr should not be mounted after stop");
}

/**
 * @brief Test multiple unmount attempts
 */
ZTEST(test_lfs_mgr, test_multiple_unmount_attempts)
{
    int rc;

    /* First unmount attempt (when not mounted) */
    rc = stop_lfs_mgr();
    zassert_equal(rc, 0, "First unmount should be safe");

    /* Second unmount attempt */
    rc = stop_lfs_mgr();
    zassert_equal(rc, 0, "Second unmount should be safe");

    /* Mount and unmount multiple times */
    rc = start_lfs_mgr();
    zassert_equal(rc, 0, "Mount should succeed");

    rc = stop_lfs_mgr();
    zassert_equal(rc, 0, "First unmount after mount should succeed");

    rc = stop_lfs_mgr();
    zassert_equal(rc, 0, "Second unmount should be safe");
}

/**
 * @brief Test file write operation
 */
ZTEST(test_lfs_mgr, test_file_write)
{
    int fd;
    ssize_t written;

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Remove file if it exists */
    remove(TEST_FILE_PATH);

    /* Open file for writing */
    fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd >= 0, "Failed to open file for writing");

    /* Write data */
    written = write(fd, TEST_CONTENT, TEST_CONTENT_LEN);
    zassert_equal(written, TEST_CONTENT_LEN, "Failed to write complete data");

    /* Close file */
    close(fd);

    /* Cleanup */
    stop_lfs_mgr();
}

/**
 * @brief Test file read operation
 */
ZTEST(test_lfs_mgr, test_file_read)
{
    int fd;
    ssize_t read_bytes;
    char buffer[64];

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Write test data first */
    fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd >= 0, "Failed to open file for writing");
    write(fd, TEST_CONTENT, TEST_CONTENT_LEN);
    close(fd);

    /* Open file for reading */
    fd = open(TEST_FILE_PATH, O_RDONLY);
    zassert_true(fd >= 0, "Failed to open file for reading");

    /* Read data */
    memset(buffer, 0, sizeof(buffer));
    read_bytes = read(fd, buffer, TEST_CONTENT_LEN);
    zassert_equal(read_bytes, TEST_CONTENT_LEN, "Failed to read complete data");

    /* Verify content */
    zassert_mem_equal(buffer, TEST_CONTENT, TEST_CONTENT_LEN,
                      "Read data does not match written data");

    /* Close file */
    close(fd);

    /* Cleanup */
    remove(TEST_FILE_PATH);
    stop_lfs_mgr();
}

/**
 * @brief Test file append operation
 */
ZTEST(test_lfs_mgr, test_file_append)
{
    int fd;
    ssize_t written;
    char buffer[64];

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Write initial data */
    fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd >= 0, "Failed to open file for writing");
    write(fd, TEST_CONTENT, 5);
    close(fd);

    /* Append more data */
    fd = open(TEST_FILE_PATH, O_WRONLY | O_APPEND);
    zassert_true(fd >= 0, "Failed to open file for append");
    written = write(fd, TEST_CONTENT + 5, TEST_CONTENT_LEN - 5);
    zassert_equal(written, TEST_CONTENT_LEN - 5, "Failed to append data");
    close(fd);

    /* Read back and verify */
    fd = open(TEST_FILE_PATH, O_RDONLY);
    zassert_true(fd >= 0, "Failed to open file for reading");
    memset(buffer, 0, sizeof(buffer));
    read(fd, buffer, TEST_CONTENT_LEN);
    zassert_mem_equal(buffer, TEST_CONTENT, TEST_CONTENT_LEN,
                      "Appended data does not match");
    close(fd);

    /* Cleanup */
    remove(TEST_FILE_PATH);
    stop_lfs_mgr();
}

/**
 * @brief Test file write-read-verify cycle
 */
ZTEST(test_lfs_mgr, test_file_write_read_verify)
{
    int fd;
    ssize_t written, read_bytes;
    char write_buf[64];
    char read_buf[64];

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Prepare test data */
    memset(write_buf, 0xAB, sizeof(write_buf));
    memset(read_buf, 0, sizeof(read_buf));

    /* Remove file if exists */
    remove(TEST_FILE_PATH);

    /* Write data */
    fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd >= 0, "Failed to open file");
    written = write(fd, write_buf, sizeof(write_buf));
    zassert_equal(written, sizeof(write_buf), "Write size mismatch");
    close(fd);

    /* Read data back */
    fd = open(TEST_FILE_PATH, O_RDONLY);
    zassert_true(fd >= 0, "Failed to open file for reading");
    read_bytes = read(fd, read_buf, sizeof(read_buf));
    zassert_equal(read_bytes, sizeof(read_buf), "Read size mismatch");
    close(fd);

    /* Verify */
    zassert_mem_equal(write_buf, read_buf, sizeof(write_buf),
                      "Write-read data mismatch");

    /* Cleanup */
    remove(TEST_FILE_PATH);
    stop_lfs_mgr();
}

/**
 * @brief Test file truncate operation
 */
ZTEST(test_lfs_mgr, test_file_truncate)
{
    int fd;
    struct stat st;

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Create and write file */
    fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd >= 0, "Failed to open file");
    write(fd, TEST_CONTENT, TEST_CONTENT_LEN);
    close(fd);

    /* Truncate file */
    int rc = truncate(TEST_FILE_PATH, 5);
    zassert_equal(rc, 0, "truncate failed");

    /* Verify size */
    rc = stat(TEST_FILE_PATH, &st);
    zassert_equal(rc, 0, "stat failed");
    zassert_equal(st.st_size, 5, "File size should be 5 after truncate");

    /* Cleanup */
    remove(TEST_FILE_PATH);
    stop_lfs_mgr();
}

/**
 * @brief Test directory operations
 */
ZTEST(test_lfs_mgr, test_directory_operations)
{
    int rc;

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Create directory */
    rc = mkdir(TEST_DIR_PATH, 0777);
    zassert_equal(rc, 0, "mkdir failed");

    /* Verify directory exists */
    struct stat st;
    rc = stat(TEST_DIR_PATH, &st);
    zassert_equal(rc, 0, "stat on directory failed");
    zassert_true(S_ISDIR(st.st_mode), "Path is not a directory");

    /* Remove directory */
    rc = rmdir(TEST_DIR_PATH);
    zassert_equal(rc, 0, "rmdir failed");

    /* Cleanup */
    stop_lfs_mgr();
}

/**
 * @brief Test file stat operation
 */
ZTEST(test_lfs_mgr, test_file_stat)
{
    int fd;
    struct stat st;

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Create file */
    fd = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd >= 0, "Failed to create file");
    write(fd, TEST_CONTENT, TEST_CONTENT_LEN);
    close(fd);

    /* Get file stats */
    int rc = stat(TEST_FILE_PATH, &st);
    zassert_equal(rc, 0, "stat failed");
    zassert_true(S_ISREG(st.st_mode), "Not a regular file");
    zassert_equal(st.st_size, TEST_CONTENT_LEN, "File size mismatch");

    /* Cleanup */
    remove(TEST_FILE_PATH);
    stop_lfs_mgr();
}

/**
 * @brief Test multiple file operations
 */
ZTEST(test_lfs_mgr, test_multiple_file_operations)
{
    int fd1, fd2;
    char buf[32];
    ssize_t n;

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Remove files if exist */
    remove(TEST_FILE_PATH);
    remove(TEST_FILE_PATH2);

    /* Create first file */
    fd1 = open(TEST_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd1 >= 0, "Failed to create first file");
    write(fd1, "File1", 5);
    close(fd1);

    /* Create second file */
    fd2 = open(TEST_FILE_PATH2, O_WRONLY | O_CREAT | O_TRUNC);
    zassert_true(fd2 >= 0, "Failed to create second file");
    write(fd2, "File2", 5);
    close(fd2);

    /* Read back first file */
    fd1 = open(TEST_FILE_PATH, O_RDONLY);
    zassert_true(fd1 >= 0, "Failed to open first file");
    n = read(fd1, buf, 5);
    zassert_equal(n, 5, "Read size mismatch");
    zassert_mem_equal(buf, "File1", 5, "First file content mismatch");
    close(fd1);

    /* Read back second file */
    fd2 = open(TEST_FILE_PATH2, O_RDONLY);
    zassert_true(fd2 >= 0, "Failed to open second file");
    n = read(fd2, buf, 5);
    zassert_equal(n, 5, "Read size mismatch");
    zassert_mem_equal(buf, "File2", 5, "Second file content mismatch");
    close(fd2);

    /* Cleanup */
    remove(TEST_FILE_PATH);
    remove(TEST_FILE_PATH2);
    stop_lfs_mgr();
}

/**
 * @brief Test seek operation
 */
ZTEST(test_lfs_mgr, test_file_seek)
{
    int fd;
    char buf[10];

    /* Ensure filesystem is mounted */
    start_lfs_mgr();

    /* Create file with known content */
    fd = open(TEST_FILE_PATH, O_RDWR | O_CREAT | O_TRUNC);
    zassert_true(fd >= 0, "Failed to open file");
    write(fd, "0123456789", 10);
    close(fd);

    /* Reopen and seek */
    fd = open(TEST_FILE_PATH, O_RDONLY);
    zassert_true(fd >= 0, "Failed to open file");

    /* Seek to position 5 */
    off_t rc = lseek(fd, 5, SEEK_SET);
    zassert_equal(rc, 5, "lseek failed");

    /* Read from position 5 */
    memset(buf, 0, sizeof(buf));
    read(fd, buf, 3);
    zassert_mem_equal(buf, "567", 3, "Seek and read mismatch");

    close(fd);

    /* Cleanup */
    remove(TEST_FILE_PATH);
    stop_lfs_mgr();
}

ZTEST_SUITE(test_lfs_mgr, NULL, NULL, NULL, NULL, NULL);
