cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(zephyr-dwm1001)

target_sources(app PRIVATE ../../main.c)
target_sources(app PRIVATE ex_11a_main.c)