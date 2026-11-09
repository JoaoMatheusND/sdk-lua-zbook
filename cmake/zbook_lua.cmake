# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE 2026
#
# zbook_lua.cmake -- application-side helper for the ZBook Lua binding.
#
# Wraps lua_zephyr's luaz.cmake so applications neither hardcode its path nor
# hand-write a `<script>_lua_setup()` hook per Lua thread:
#
#   cmake_minimum_required(VERSION 3.20.0)
#   include(<path-to-sdk-lua-zbook>/cmake/zbook_lua.cmake)
#   zbook_lua_define_thread(src/main.lua)      # before find_package(Zephyr)
#   find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#   project(my_app)
#   zbook_lua_generate()                       # after find_package(Zephyr)
#
# Every thread defined here gets the global `zbook` table (and
# require("zbook")) before its script runs.
#
# lua_zephyr location, first match wins:
#   1. -DLUA_ZEPHYR_DIR=<path> / set(LUA_ZEPHYR_DIR ...) before the include
#   2. LUA_ZEPHYR_DIR environment variable
#   3. <west workspace root>/deps/modules/lua/lua_zephyr (path pinned in
#      this repo's west.yml)

include_guard(GLOBAL)

get_filename_component(ZBOOK_LUA_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(NOT DEFINED LUA_ZEPHYR_DIR AND DEFINED ENV{LUA_ZEPHYR_DIR})
  set(LUA_ZEPHYR_DIR "$ENV{LUA_ZEPHYR_DIR}")
endif()

if(NOT DEFINED LUA_ZEPHYR_DIR)
  # Walk up from this repo until the west workspace root (the dir holding .west/).
  set(_zbook_lua_dir "${ZBOOK_LUA_DIR}")
  while(NOT EXISTS "${_zbook_lua_dir}/.west")
    get_filename_component(_zbook_lua_parent "${_zbook_lua_dir}" DIRECTORY)
    if(_zbook_lua_parent STREQUAL _zbook_lua_dir)
      message(FATAL_ERROR
        "zbook_lua: no west workspace found above ${ZBOOK_LUA_DIR}; "
        "set LUA_ZEPHYR_DIR to the lua_zephyr checkout.")
    endif()
    set(_zbook_lua_dir "${_zbook_lua_parent}")
  endwhile()
  set(LUA_ZEPHYR_DIR "${_zbook_lua_dir}/deps/modules/lua/lua_zephyr")
endif()

if(NOT EXISTS "${LUA_ZEPHYR_DIR}/luaz.cmake")
  message(FATAL_ERROR
    "zbook_lua: luaz.cmake not found in ${LUA_ZEPHYR_DIR}; run `west update` "
    "or set LUA_ZEPHYR_DIR.")
endif()

# Pre-Zephyr half of luaz.cmake: luaz_define_*_thread() + per-thread Kconfig.
# The post-Zephyr half is included again by the lua_zephyr module itself.
include("${LUA_ZEPHYR_DIR}/luaz.cmake")

# zbook_lua_define_thread(<path/to/script.lua>)
#
# Define a source-embedded Lua thread with `zbook` preloaded. Call before
# find_package(Zephyr).
macro(zbook_lua_define_thread _path)
  luaz_define_source_thread("${_path}")
  list(APPEND ZBOOK_LUA_THREADS "${_path}")
endmacro()

# zbook_lua_generate()
#
# Generate one `<script>_lua_setup()` hook per zbook_lua_define_thread() call
# (overriding lua_zephyr's weak default) and then every Lua thread. Call after
# find_package(Zephyr) and project().
function(zbook_lua_generate)
  foreach(_path ${ZBOOK_LUA_THREADS})
    cmake_path(GET _path STEM FILE_NAME)
    set(_out "${CMAKE_CURRENT_BINARY_DIR}/zbook_lua/${FILE_NAME}_zbook_lua_setup.c")
    configure_file("${ZBOOK_LUA_DIR}/templates/zbook_lua_setup.c.in" "${_out}" @ONLY)
    target_sources(app PRIVATE "${_out}")
  endforeach()

  luaz_generate_threads()
endfunction()
