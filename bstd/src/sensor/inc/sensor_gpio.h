#pragma once

/*
 * GPIO sensor source.
 *
 * This module is a pluggable "sensor source" that feeds the sensor core
 * (sensor.c) via insert_sensor_data(). It is interrupt-driven:
 *   - ISR only signals a semaphore (never does I/O or logging)
 *   - A dedicated thread debounces, reads the pin, and publishes
 *     only on state change.
 *
 * The core (sensor.c) is GPIO-agnostic; it only knows about
 * insert_sensor_data(). This keeps hardware concerns isolated.
 */

int init_sensor_gpio(void);
int start_sensor_gpio(void);
int stop_sensor_gpio(void);