BSTD - Board System Test & Development Framework

Overview
********

BSTD is a modular embedded firmware framework built on Zephyr RTOS, designed for
ESP32-S3 and other supported boards. It provides a structured, event-driven
architecture leveraging Zephyr's kernel primitives, memory management, and
device tree for pluggable subsystems covering BLE communication, sensor
management, network connectivity, data compression, and device configuration.

Technical Architecture
-----------------------

- **Zephyr Kernel Primitives**: Uses ``k_msgq`` for thread-safe inter-module
  messaging, ``K_FOREVER``/``K_NO_WAIT`` timeout semantics, and cooperative
  scheduling with a single main thread dispatching events to subsystems.
- **Modular Subsystem Design**: Nine independently configurable modules
  (``CONFIG_*`` toggles in Kconfig) initialized via function pointer tables with
  ``init()``, ``start()``, and ``stop()`` lifecycle callbacks; modules are
  conditionally compiled based on ``prj.conf``.
- **Zephyr Build System Integration**: Built with West/meta-tooling, CMake
  multi-image support, and board-specific overlays (e.g., ESP32-S3 DevKitC
  procpu overlay).
- **Subsystems**:
  - ``ble`` - BLE advertising, connection, and GATT services
  - ``commu`` - Dual-client communication protocol (Client A/B)
  - ``feature`` - Feature tasks, state handlers, pattern matching, and
    ``wzip`` lossless compression (Golomb-Rice + pattern matching + TLV)
  - ``netwrk`` - WiFi, HTTP, and network task management
  - ``sensor`` / ``gpio_mgr`` - Hardware abstraction layers
  - ``nvs_mgr`` / ``lfs_mgr`` - Non-volatile and LittleFS storage backends
  - ``dev_sett`` - Device settings management
  - ``shell`` - Interactive CLI via Zephyr shell

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    Hello World! x86

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.


build command 
uttam@uttam:~/zephyrproject$ source ~/zephyr-env/bin/activate
(zephyr-env) uttam@uttam:~/zephyrproject$ west build -p always -b esp32s3_devkitc/esp32s3/procpu bstd
(zephyr-env) uttam@uttam:~/zephyrproject$ west flash
(zephyr-env) uttam@uttam:~/zephyrproject$ west espressif monitor



# Run all lfs_mgr tests
west twister -p native_sim -T bstd/tests/lfs_mgr

# Or use west directly
cd bstd && west build -b native_sim -T tests/lfs_mgr
