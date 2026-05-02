#pragma once
#include <stdio.h>
#define DEBUG   1
#define INFO    2
#define WARN    3
#define ERROR   4

#define LOG_LEVEL INFO

#if LOG_LEVEL <= DEBUG
#define log_d(fmt, ...) printf(" D %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_i(fmt, ...) printf(" I %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_w(fmt, ...) printf(" W %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_e(fmt, ...) printf(" E %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_c(fmt, ...) printf(" C %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#elif LOG_LEVEL <= INFO
#define log_d(fmt, ...) 
#define log_i(fmt, ...) printf(" I %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_w(fmt, ...) printf(" W %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_e(fmt, ...) printf(" E %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_c(fmt, ...) printf(" C %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#elif LOG_LEVEL <= WARN
#define log_d(fmt, ...) 
#define log_i(fmt, ...) 
#define log_w(fmt, ...) printf(" W %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_e(fmt, ...) printf(" E %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#define log_c(fmt, ...) printf(" C %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#elif LOG_LEVEL <= ERROR
#define log_d(fmt, ...) 
#define log_i(fmt, ...) 
#define log_w(fmt, ...) 
#define log_e(fmt, ...) printf(" E %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__) 
#define log_c(fmt, ...) printf(" C %s(%d): "fmt"\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define log_d(fmt, ...)
#define log_i(fmt, ...)
#define log_w(fmt, ...)
#define log_e(fmt, ...)
#define log_c(fmt, ...)
#endif