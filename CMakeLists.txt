# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE 2026

if(CONFIG_ZBOOK_LUA)
  zephyr_include_directories(include)

  zephyr_library()

  # Every binding source is fully wrapped in #ifdef CONFIG_ZBOOK_LUA_<PERIPH>,
  # so globbing once is enough: adding a binding never touches this file.
  file(GLOB_RECURSE ZBOOK_LUA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
  zephyr_library_sources(${ZBOOK_LUA_SOURCES})

  # Collects ZBOOK_LUA_BINDING_DEFINE() entries into one iterable ROM section.
  zephyr_linker_sources(SECTIONS zbook_lua_binding.ld)
endif()
