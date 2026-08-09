#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <time.h>
#include "sensor.h"
#include "sensor_gpio.h"
#include "loggers.h"

/* GPIO input node defined in the board overlay
 * (see bstd/boards/esp32s3_devkitc_procpu.overlay).
 */
#define SENSOR_IN_NODE DT_NODELABEL(sensor_in)

/* Debounce interval: let the line settle before trusting a read. */
#define SENSOR_DEBOUNCE_MS 30

/* GPIO device + pin spec, resolved from the devicetree at build time. */
static const struct gpio_dt_spec sensor_gpio = GPIO_DT_SPEC_GET(SENSOR_IN_NODE, gpios);

/* ISR signalling + thread synchronization. */
static struct gpio_callback sensor_gpio_cb;
static struct k_sem sensor_irq_sem;

/* Last published value; -1 forces the first read to be published. */
static int sensor_last_val = -1;

/*
 * ISR context: MUST be minimal and non-blocking.
 * Only signal the semaphore; all I/O, logging, and callbacks
 * happen in the thread below.
 */
static void sensor_gpio_isr(const struct device *dev,
                            struct gpio_callback *cb, uint32_t pins)
{
    k_sem_give(&sensor_irq_sem);
}

/*
 * Thread context: debounce, read, and publish only on state change.
 * This keeps the ISR fast and safe.
 */
static void sensor_gpio_thread(void *arg1, void *arg2, void *arg3)
{
    log_i("sensor_gpio_thread started");
    while (1) {
        k_sem_take(&sensor_irq_sem, K_FOREVER);

        /* Let the line settle before trusting the read. */
        k_msleep(SENSOR_DEBOUNCE_MS);

        int val = gpio_pin_get_dt(&sensor_gpio);
        if (val < 0) {
            log_e("GPIO read failed: %d", val);
            continue;
        }

        /* Publish only on state change to avoid noisy duplicate events. */
        if (val != sensor_last_val) {
            sensor_last_val = val;

            sensor_out_t out = {0, };
            out.type = SENS_TYPE_S;
            out.timestamp = time(NULL);
            out.length = 1;
            out.data[0] = (unsigned char) val;

            log_i("GPIO sensor value: %d", val);
            insert_sensor_data(SENS_TYPE_S, sizeof(sensor_out_t), &out);
        }
    }
}

K_THREAD_DEFINE(sensor_gpio_th, 2048, sensor_gpio_thread, NULL, NULL, NULL, 10, 0, 0);

int init_sensor_gpio(void)
{
    log_i("Init");

    if (!gpio_is_ready_dt(&sensor_gpio)) {
        log_e("GPIO device %s not ready", sensor_gpio.port->name);
        return -ENODEV;
    }

    /* Configure the pin as input. Pull-up / active-low flags come
     * from the devicetree node via sensor_gpio.dt_flags.
     */
    int ret = gpio_pin_configure_dt(&sensor_gpio, GPIO_INPUT);
    if (ret < 0) {
        log_e("GPIO configure failed: %d", ret);
        return ret;
    }

    k_sem_init(&sensor_irq_sem, 0, 1);

    gpio_init_callback(&sensor_gpio_cb, sensor_gpio_isr, BIT(sensor_gpio.pin));
    ret = gpio_add_callback(sensor_gpio.port, &sensor_gpio_cb);
    if (ret < 0) {
        log_e("GPIO add callback failed: %d", ret);
        return ret;
    }

    log_i("GPIO input ready: %s pin %u", sensor_gpio.port->name, sensor_gpio.pin);
    return 0;
}

int start_sensor_gpio(void)
{
    log_i("Start");

    /* Enable interrupts on both edges so every state change is caught. */
    int ret = gpio_pin_interrupt_configure_dt(&sensor_gpio, GPIO_INT_EDGE_BOTH);
    if (ret < 0) {
        log_e("GPIO interrupt configure failed: %d", ret);
        return ret;
    }

    return 0;
}

int stop_sensor_gpio(void)
{
    log_i("Stop");

    /* Disable interrupts for clean shutdown. */
    gpio_pin_interrupt_configure_dt(&sensor_gpio, GPIO_DISCONNECTED);
    return 0;
}