cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(zephyr-dwm1001)


add_definitions(-DEX_05B_DEF)

target_sources(app PRIVATE ../../main.c)
target_sources(app PRIVATE ex_05c_main.c)

target_sources(app PRIVATE ../../decadriver/deca_device.c)
target_sources(app PRIVATE ../../decadriver/deca_params_init.c)

target_sources(app PRIVATE ../../platform/port.c)
target_sources(app PRIVATE ../../platform/deca_mutex.c)
target_sources(app PRIVATE ../../platform/deca_range_tables.c)
target_sources(app PRIVATE ../../platform/deca_sleep.c)
target_sources(app PRIVATE ../../platform/deca_spi.c)
target_sources(app PRIVATE ../../platform/port.c)

target_sources(app PRIVATE ../../ble/ble_dwm1001.c)


target_sources(app PRIVATE
  ${app_sources}
  $ENV{ZEPHYR_BASE}/samples/bluetooth/gatt/hrs.c
  $ENV{ZEPHYR_BASE}/samples/bluetooth/gatt/dps.c
  )

target_include_directories(app PRIVATE ../../decadriver/)
target_include_directories(app PRIVATE ../../platform/)
target_include_directories(app PRIVATE ../../compiler/)
target_include_directories(app PRIVATE ../../ble/)


zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth/gatt/)
 