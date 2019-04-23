cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(zephyr-dwm1001)

target_sources(app PRIVATE ../../main.c)
target_sources(app PRIVATE ex_12a_main.c)

target_sources(app PRIVATE ../../platform/port.c)

target_sources(app PRIVATE ../../ble/ble_dwm1001.c)

target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/samples/bluetooth/gatt/dps.c)

  
target_include_directories(app PRIVATE ../../decadriver/)
target_include_directories(app PRIVATE ../../platform/)
target_include_directories(app PRIVATE ../../compiler/)
target_include_directories(app PRIVATE ../../ble/)


zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth/gatt/)
