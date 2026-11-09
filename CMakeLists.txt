# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED BOARD)
    set(BOARD zbook@p2/rp2350b/m33)
endif()

# sdk-lua-zbook checkout; default matches the `path:` in west.yml.
if(NOT DEFINED ZBOOK_LUA_DIR)
    set(ZBOOK_LUA_DIR "${CMAKE_CURRENT_LIST_DIR}/../wrapper/lua")
endif()

include(${ZBOOK_LUA_DIR}/cmake/zbook_lua.cmake)

# One Lua thread per script; each gets the global `zbook` table.
zbook_lua_define_thread(src/game.lua)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fw_lua_zbook)

zbook_lua_generate()
