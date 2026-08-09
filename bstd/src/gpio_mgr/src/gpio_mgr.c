#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "gpio_mgr.h"
#include "loggers.h"

/* ================================================================
 * Design: Singleton + Registry + Devicetree-enumerated pins
 *
 * - Singleton: one global gpio_mgr_t instance
 * - Registry: array of gpio_pin_cfg_t, one entry per physical pin
 * - Devicetree: all pins declared in board overlay under gpio_mgr node
 * - ISR: single callback scans pins bitmask, dispatches to per-pin cb
 * ================================================================ */

/* ==================== INTERNAL STATE ==================== */

struct gpio_mgr_s {
    bool initialized;
    bool started;

    /* Pin registry */
    gpio_pin_cfg_t pins[GPIO_MGR_MAX_PINS];
    uint32_t pin_count;

    /* Shared GPIO callback for all input pins */
    struct gpio_callback gpio_cb;

    /* Input notification semaphore */
    struct k_sem input_sem;

    /* Blink control */
    struct k_thread blink_thr;
    k_tid_t blink_tid;
    struct k_sem blink_stop_sem;
    bool blink_active;
    gpio_blink_pattern_t blink_pattern;
    uint32_t blink_repeat;
};

static struct gpio_mgr_s gpio_mgr_inst;

/* ==================== HELPERS ==================== */

static gpio_pin_cfg_t *find_pin(uint32_t pin)
{
    for (uint32_t i = 0; i < gpio_mgr_inst.pin_count; i++) {
        if (gpio_mgr_inst.pins[i].pin == pin) {
            return &gpio_mgr_inst.pins[i];
        }
    }
    return NULL;
}

/* ==================== INPUT ISR ==================== */

static void gpio_mgr_isr(const struct device *dev,
                         struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);
    /* Signal thread to process input changes */
    k_sem_give(&gpio_mgr_inst.input_sem);
}

/* ==================== INPUT THREAD ==================== */

static void gpio_mgr_input_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    log_i("gpio_mgr_input_thread started");

    while (1) {
        k_sem_take(&gpio_mgr_inst.input_sem, K_FOREVER);

        /* Scan all registered input pins and dispatch callbacks */
        for (uint32_t i = 0; i < gpio_mgr_inst.pin_count; i++) {
            gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[i];

            if (p->dir != GPIO_DIR_INPUT) {
                continue;
            }
            if (!p->int_enabled || !p->cb) {
                continue;
            }

            int val = gpio_pin_get_dt(&p->spec);
            if (val < 0) {
                log_e("GPIO%u read failed: %d", p->pin, val);
                continue;
            }

            p->cb(p->pin, (gpio_state_t)val, p->cb_user_data);
        }
    }
}

K_THREAD_DEFINE(gpio_mgr_input_th, 2048, gpio_mgr_input_thread, NULL, NULL, NULL, 10, 0, 0);

/* ==================== BLINK THREAD ==================== */

static void gpio_mgr_blink_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    gpio_blink_pattern_t pattern = gpio_mgr_inst.blink_pattern;
    uint32_t repeat = gpio_mgr_inst.blink_repeat;

    log_i("gpio_mgr_blink_thread started: pattern=%u repeat=%u", pattern, repeat);

    while (gpio_mgr_inst.blink_active) {
        switch (pattern) {
        case GPIO_BLINK_SOS: {
            /* ... --- ... */
            const uint16_t dot = 100;
            const uint16_t dash = 300;
            const uint16_t gap = 100;
            const uint16_t letter_gap = 300;
            const uint16_t word_gap = 700;

            /* S: dot dot dot */
            for (uint32_t r = 0; r < repeat && gpio_mgr_inst.blink_active; r++) {
                for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                    gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                    if (p->dir != GPIO_DIR_OUTPUT) continue;
                    gpio_pin_set_dt(&p->spec, 1);
                }
                k_msleep(dot);
                for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                    gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                    if (p->dir != GPIO_DIR_OUTPUT) continue;
                    gpio_pin_set_dt(&p->spec, 0);
                }
                k_msleep(gap);
            }
            k_msleep(letter_gap);

            /* O: dash dash dash */
            for (uint32_t r = 0; r < repeat && gpio_mgr_inst.blink_active; r++) {
                for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                    gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                    if (p->dir != GPIO_DIR_OUTPUT) continue;
                    gpio_pin_set_dt(&p->spec, 1);
                }
                k_msleep(dash);
                for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                    gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                    if (p->dir != GPIO_DIR_OUTPUT) continue;
                    gpio_pin_set_dt(&p->spec, 0);
                }
                k_msleep(gap);
            }
            k_msleep(letter_gap);

            /* S: dot dot dot */
            for (uint32_t r = 0; r < repeat && gpio_mgr_inst.blink_active; r++) {
                for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                    gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                    if (p->dir != GPIO_DIR_OUTPUT) continue;
                    gpio_pin_set_dt(&p->spec, 1);
                }
                k_msleep(dot);
                for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                    gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                    if (p->dir != GPIO_DIR_OUTPUT) continue;
                    gpio_pin_set_dt(&p->spec, 0);
                }
                k_msleep(gap);
            }
            k_msleep(word_gap);
            break;
        }

        case GPIO_BLINK_SEQUENTIAL: {
            const uint16_t on_time = 200;
            const uint16_t off_time = 100;

            for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count && gpio_mgr_inst.blink_active; pin_idx++) {
                gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                if (p->dir != GPIO_DIR_OUTPUT) continue;

                gpio_pin_set_dt(&p->spec, 1);
                k_msleep(on_time);
                gpio_pin_set_dt(&p->spec, 0);
                k_msleep(off_time);
            }
            break;
        }

        case GPIO_BLINK_ALL_TOGGLE: {
            const uint16_t period = 500;

            for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                if (p->dir != GPIO_DIR_OUTPUT) continue;
                gpio_pin_set_dt(&p->spec, 1);
            }
            k_msleep(period);

            for (uint32_t pin_idx = 0; pin_idx < gpio_mgr_inst.pin_count; pin_idx++) {
                gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[pin_idx];
                if (p->dir != GPIO_DIR_OUTPUT) continue;
                gpio_pin_set_dt(&p->spec, 0);
            }
            k_msleep(period);
            break;
        }

        default:
            break;
        }
    }

    /* Cleanup: turn off all outputs */
    for (uint32_t i = 0; i < gpio_mgr_inst.pin_count; i++) {
        gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[i];
        if (p->dir == GPIO_DIR_OUTPUT) {
            gpio_pin_set_dt(&p->spec, 0);
        }
    }

    log_i("gpio_mgr_blink_thread stopped");
    gpio_mgr_inst.blink_tid = 0;
}

/* ==================== PUBLIC API ==================== */

gpio_mgr_t *gpio_mgr_get(void)
{
    return &gpio_mgr_inst;
}

int init_gpio_mgr(void)
{
    log_i("init_gpio_mgr");

    if (gpio_mgr_inst.initialized) {
        log_w("Already initialized");
        return 0;
    }

    memset(&gpio_mgr_inst, 0, sizeof(gpio_mgr_inst));
    k_sem_init(&gpio_mgr_inst.input_sem, 0, 1);
    k_sem_init(&gpio_mgr_inst.blink_stop_sem, 0, 1);

    gpio_mgr_inst.initialized = true;
    log_i("gpio_mgr initialized");
    return 0;
}

int start_gpio_mgr(void)
{
    log_i("start_gpio_mgr");

    if (!gpio_mgr_inst.initialized) {
        log_e("Not initialized");
        return -EINVAL;
    }
    if (gpio_mgr_inst.started) {
        log_w("Already started");
        return 0;
    }

    /* Configure all registered pins */
    for (uint32_t i = 0; i < gpio_mgr_inst.pin_count; i++) {
        gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[i];

        if (!gpio_is_ready_dt(&p->spec)) {
            log_e("GPIO%u device not ready", p->pin);
            continue;
        }

        int ret;
        if (p->dir == GPIO_DIR_INPUT) {
            ret = gpio_pin_configure_dt(&p->spec, GPIO_INPUT);
        } else {
            ret = gpio_pin_configure_dt(&p->spec, GPIO_OUTPUT);
            if (ret == 0 && p->init_val == GPIO_STATE_HIGH) {
                gpio_pin_set_dt(&p->spec, 1);
            }
        }

        if (ret < 0) {
            log_e("GPIO%u configure failed: %d", p->pin, ret);
            continue;
        }

        log_i("GPIO%u configured as %s", p->pin,
              p->dir == GPIO_DIR_INPUT ? "input" : "output");
    }

    /* Register single shared callback for all input pins */
    if (gpio_mgr_inst.pin_count > 0) {
        gpio_init_callback(&gpio_mgr_inst.gpio_cb, gpio_mgr_isr, 0);
        /* Bitmask built dynamically below */
    }

    gpio_mgr_inst.started = true;
    log_i("gpio_mgr started");
    return 0;
}

int stop_gpio_mgr(void)
{
    log_i("stop_gpio_mgr");

    if (!gpio_mgr_inst.started) {
        return 0;
    }

    /* Stop blink first */
    gpio_mgr_blink_stop();

    /* Disable interrupts on all input pins */
    for (uint32_t i = 0; i < gpio_mgr_inst.pin_count; i++) {
        gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[i];
        if (p->dir == GPIO_DIR_INPUT && p->int_enabled) {
            gpio_pin_interrupt_configure_dt(&p->spec, GPIO_DISCONNECTED);
        }
    }

    gpio_mgr_inst.started = false;
    log_i("gpio_mgr stopped");
    return 0;
}

/* ==================== RUNTIME API ==================== */

int gpio_mgr_write(uint32_t pin, gpio_state_t val)
{
    gpio_pin_cfg_t *p = find_pin(pin);
    if (!p || p->dir != GPIO_DIR_OUTPUT) {
        log_e("gpio_mgr_write: GPIO%u invalid or not output", pin);
        return -EINVAL;
    }

    return gpio_pin_set_dt(&p->spec, val ? 1 : 0);
}

int gpio_mgr_read(uint32_t pin)
{
    gpio_pin_cfg_t *p = find_pin(pin);
    if (!p) {
        log_e("gpio_mgr_read: GPIO%u not found", pin);
        return -EINVAL;
    }

    return gpio_pin_get_dt(&p->spec);
}

int gpio_mgr_toggle(uint32_t pin)
{
    gpio_pin_cfg_t *p = find_pin(pin);
    if (!p || p->dir != GPIO_DIR_OUTPUT) {
        log_e("gpio_mgr_toggle: GPIO%u invalid or not output", pin);
        return -EINVAL;
    }

    int val = gpio_pin_get_dt(&p->spec);
    if (val < 0) {
        return val;
    }
    return gpio_pin_set_dt(&p->spec, !val);
}

int gpio_mgr_set_direction(uint32_t pin, gpio_dir_t dir)
{
    gpio_pin_cfg_t *p = find_pin(pin);
    if (!p) {
        log_e("gpio_mgr_set_direction: GPIO%u not found", pin);
        return -EINVAL;
    }

    if (gpio_mgr_inst.started) {
        log_e("Cannot change direction while running");
        return -EBUSY;
    }

    p->dir = dir;
    p->int_enabled = (dir == GPIO_DIR_INPUT);
    return 0;
}

int gpio_mgr_register_input_cb(uint32_t pin, gpio_input_cb_t cb, void *user_data)
{
    gpio_pin_cfg_t *p = find_pin(pin);
    if (!p) {
        log_e("gpio_mgr_register_input_cb: GPIO%u not found", pin);
        return -EINVAL;
    }

    if (p->dir != GPIO_DIR_INPUT) {
        log_e("GPIO%u is not configured as input", pin);
        return -EINVAL;
    }

    p->cb = cb;
    p->cb_user_data = user_data;
    log_i("GPIO%u callback registered", pin);
    return 0;
}

int gpio_mgr_write_all(const gpio_state_t *vals, uint32_t count)
{
    if (!vals || count == 0) {
        return -EINVAL;
    }

    int ret = 0;
    for (uint32_t i = 0; i < count && i < gpio_mgr_inst.pin_count; i++) {
        gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[i];
        if (p->dir != GPIO_DIR_OUTPUT) {
            continue;
        }
        int r = gpio_pin_set_dt(&p->spec, vals[i] ? 1 : 0);
        if (r < 0) ret = r;
    }
    return ret;
}

int gpio_mgr_read_all(gpio_state_t *vals, uint32_t count)
{
    if (!vals || count == 0) {
        return -EINVAL;
    }

    int ret = 0;
    for (uint32_t i = 0; i < count && i < gpio_mgr_inst.pin_count; i++) {
        gpio_pin_cfg_t *p = &gpio_mgr_inst.pins[i];
        if (p->dir != GPIO_DIR_INPUT) {
            vals[i] = GPIO_STATE_LOW;
            continue;
        }
        int v = gpio_pin_get_dt(&p->spec);
        vals[i] = (v > 0) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
        if (v < 0) ret = v;
    }
    return ret;
}

/* ==================== BLINK API ==================== */

int gpio_mgr_blink(gpio_blink_pattern_t pattern, uint32_t repeat)
{
    if (gpio_mgr_inst.blink_active) {
        gpio_mgr_blink_stop();
    }

    gpio_mgr_inst.blink_pattern = pattern;
    gpio_mgr_inst.blink_repeat = repeat;
    gpio_mgr_inst.blink_active = true;

    gpio_mgr_inst.blink_tid = k_thread_create(
        &gpio_mgr_inst.blink_thr,
        NULL,
        K_THREAD_STACK_SIZEOF(gpio_mgr_inst.blink_thr),
        gpio_mgr_blink_thread,
        NULL, NULL, NULL,
        5, 0, K_NO_WAIT);

    if (!gpio_mgr_inst.blink_tid) {
        log_e("Failed to create blink thread");
        gpio_mgr_inst.blink_active = false;
        return -ENOMEM;
    }

    log_i("Blink started: pattern=%u repeat=%u", pattern, repeat);
    return 0;
}

int gpio_mgr_blink_stop(void)
{
    if (!gpio_mgr_inst.blink_active) {
        return 0;
    }

    gpio_mgr_inst.blink_active = false;

    /* Wake thread if sleeping */
    k_sem_give(&gpio_mgr_inst.blink_stop_sem);

    if (gpio_mgr_inst.blink_tid) {
        k_thread_join(gpio_mgr_inst.blink_tid, K_MSEC(2000));
    }

    log_i("Blink stopped");
    return 0;
}

/* ==================== PIN REGISTRATION ==================== */

/* Called during init or externally to register pins from devicetree.
 * This function is exported so board/overlay code can add pins.
 */
int gpio_mgr_register_pin(const gpio_pin_cfg_t *cfg)
{
    if (!cfg || !gpio_mgr_inst.initialized) {
        return -EINVAL;
    }

    if (gpio_mgr_inst.pin_count >= GPIO_MGR_MAX_PINS) {
        log_e("gpio_mgr pin registry full");
        return -ENOMEM;
    }

    gpio_mgr_inst.pins[gpio_mgr_inst.pin_count++] = *cfg;
    log_i("Registered GPIO%u", cfg->pin);
    return 0;
}

/* ==================== DEVTREE ENUMERATION (Zephyr-specific) ==================== */

/* Pins are registered via board overlay / Kconfig.
 * Devicetree enumeration placeholder removed to avoid unused warning.
 */
