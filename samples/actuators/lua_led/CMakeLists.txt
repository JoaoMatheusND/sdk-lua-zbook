# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE 2026

cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED BOARD)
    set(BOARD zbook@p2/rp2350b/m33)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/zbook_lua.cmake)
zbook_lua_define_thread(src/led.lua)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sample_zbook_lua_led)

zbook_lua_generate()
