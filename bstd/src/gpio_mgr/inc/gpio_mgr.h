#pragma once

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <stdint.h>

/* Max pins supported by this module */
#define GPIO_MGR_MAX_PINS    54

/* Pin direction */
typedef enum {
    GPIO_DIR_INPUT = 0,
    GPIO_DIR_OUTPUT = 1,
} gpio_dir_t;

/* Pin state */
typedef enum {
    GPIO_STATE_LOW = 0,
    GPIO_STATE_HIGH = 1,
} gpio_state_t;

/* Callback for input pin state changes */
typedef void (*gpio_input_cb_t)(uint32_t pin, gpio_state_t new_state, void *user_data);

/* Pin configuration */
typedef struct {
    uint32_t pin;               /* GPIO pin number (e.g. 3 for GPIO3) */
    gpio_dir_t dir;             /* Input or output */
    const char *label;          /* Human-readable name */
    gpio_state_t init_val;      /* Initial value for outputs */
    bool int_enabled;           /* Interrupt enabled for inputs */
    gpio_input_cb_t cb;         /* Callback for input changes */
    void *cb_user_data;         /* User data passed to callback */
    struct gpio_dt_spec spec;   /* Resolved devicetree spec */
} gpio_pin_cfg_t;

/* Global GPIO manager handle */
typedef struct gpio_mgr_s gpio_mgr_t;

/* Module lifecycle (matches BSTD main.c LOOKUP pattern) */
int init_gpio_mgr(void);
int start_gpio_mgr(void);
int stop_gpio_mgr(void);

/* Runtime API - easy to use from any module */
int gpio_mgr_write(uint32_t pin, gpio_state_t val);
int gpio_mgr_read(uint32_t pin);
int gpio_mgr_toggle(uint32_t pin);
int gpio_mgr_set_direction(uint32_t pin, gpio_dir_t dir);

/* Register an input callback for a pin */
int gpio_mgr_register_input_cb(uint32_t pin, gpio_input_cb_t cb, void *user_data);

/* Bulk operations */
int gpio_mgr_write_all(const gpio_state_t *vals, uint32_t count);
int gpio_mgr_read_all(gpio_state_t *vals, uint32_t count);

/* Blink pattern */
typedef enum {
    GPIO_BLINK_SOS = 0,         /* SOS pattern: ... --- ... */
    GPIO_BLINK_SEQUENTIAL,      /* 1,2,3... */
    GPIO_BLINK_ALL_TOGGLE,      /* All toggle together */
    GPIO_BLINK_MAX
} gpio_blink_pattern_t;

int gpio_mgr_blink(gpio_blink_pattern_t pattern, uint32_t repeat);
int gpio_mgr_blink_stop(void);

/* Get the singleton instance */
gpio_mgr_t *gpio_mgr_get(void);