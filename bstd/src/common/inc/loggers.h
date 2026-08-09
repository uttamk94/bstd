#pragma once
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <time.h>

#define DEBUG   1
#define INFO    2
#define WARN    3
#define ERROR   4

#define LOG_LEVEL INFO

/* Helper function to format timestamp */
static inline void format_timestamp(char *buf, size_t buf_size)
{
    time_t now;
    struct tm *tm_info;
    
    /* Get current time (will be 1970 if RTC not set) */
    time(&now);
    tm_info = gmtime(&now);
    
    /* Format: YY:MM:DD HH:MM:SS:MMM */
    snprintf(buf, buf_size, "%02d:%02d:%02d %02d:%02d:%02d:%03d",
             tm_info->tm_year % 100,      /* YY */
             tm_info->tm_mon + 1,          /* MM */
             tm_info->tm_mday,             /* DD */
             tm_info->tm_hour,             /* HH */
             tm_info->tm_min,              /* MM */
             tm_info->tm_sec,              /* SS */
             0);                           /* MMM - no milliseconds with time() */
}

/* Base macro for logging with timestamp */
#define LOG_PRINT(level, fmt, ...) \
    do { \
        char ts_buf[32]; \
        format_timestamp(ts_buf, sizeof(ts_buf)); \
        printk("%s %c %.16s(%d): "fmt"\n", ts_buf, level, __func__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#if LOG_LEVEL <= DEBUG
#define log_d(fmt, ...) LOG_PRINT('D', fmt, ##__VA_ARGS__)
#define log_i(fmt, ...) LOG_PRINT('I', fmt, ##__VA_ARGS__)
#define log_w(fmt, ...) LOG_PRINT('W', fmt, ##__VA_ARGS__)
#define log_e(fmt, ...) LOG_PRINT('E', fmt, ##__VA_ARGS__)
#define log_c(fmt, ...) LOG_PRINT('C', fmt, ##__VA_ARGS__)
#elif LOG_LEVEL <= INFO
#define log_d(fmt, ...)
#define log_i(fmt, ...) LOG_PRINT('I', fmt, ##__VA_ARGS__)
#define log_w(fmt, ...) LOG_PRINT('W', fmt, ##__VA_ARGS__)
#define log_e(fmt, ...) LOG_PRINT('E', fmt, ##__VA_ARGS__)
#define log_c(fmt, ...) LOG_PRINT('C', fmt, ##__VA_ARGS__)
#elif LOG_LEVEL <= WARN
#define log_d(fmt, ...)
#define log_i(fmt, ...)
#define log_w(fmt, ...) LOG_PRINT('W', fmt, ##__VA_ARGS__)
#define log_e(fmt, ...) LOG_PRINT('E', fmt, ##__VA_ARGS__)
#define log_c(fmt, ...) LOG_PRINT('C', fmt, ##__VA_ARGS__)
#elif LOG_LEVEL <= ERROR
#define log_d(fmt, ...)
#define log_i(fmt, ...)
#define log_w(fmt, ...)
#define log_e(fmt, ...) LOG_PRINT('E', fmt, ##__VA_ARGS__)
#define log_c(fmt, ...) LOG_PRINT('C', fmt, ##__VA_ARGS__)
#else
#define log_d(fmt, ...)
#define log_i(fmt, ...)
#define log_w(fmt, ...)
#define log_e(fmt, ...)
#define log_c(fmt, ...)
#endif

#define LOG_TAG_LEN 24