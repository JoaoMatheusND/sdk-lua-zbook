/*************************************************************************
 * @file zbook_lua.h
 *
 * @brief Public entry points of the ZBook Lua binding.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_LUA_H
#define ZBOOK_LUA_H

#include <lua.h>

/**
 * @brief Open the `zbook` Lua library.
 *
 * Builds zbook.<category>.<name> from every binding registered with
 * ZBOOK_LUA_BINDING_DEFINE() and enabled in Kconfig.
 *
 * @param L Lua state.
 * @return 1 (the library table is on the stack).
 */
int luaopen_zbook(lua_State *L);

/**
 * @brief Register `zbook` in a Lua state: global `zbook` and
 * require("zbook").
 *
 * Meant to be called from a lua_zephyr `<script>_lua_setup()` hook; the
 * zbook_lua_generate() CMake helper generates those hooks automatically.
 *
 * @param L Lua state.
 * @retval 0 Always.
 */
int zbook_lua_setup(lua_State *L);

#endif /* ZBOOK_LUA_H */
