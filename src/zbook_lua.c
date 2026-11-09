/*************************************************************************
 * @file zbook_lua.c
 *
 * @brief Builds the `zbook` Lua table from the registered bindings.
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#include "zbook_lua.h"
#include "zbook_lua_binding.h"

#include <lauxlib.h>

/**
 * @brief Push zbook[category], creating it if missing.
 *
 * @param L Lua state, with the zbook table at the top of the stack.
 * @param category Category name.
 */
static void push_category(lua_State *L, const char *category)
{
	if (lua_getfield(L, -1, category) == LUA_TTABLE) {
		return;
	}

	lua_pop(L, 1);
	lua_newtable(L);
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, category);
}

int luaopen_zbook(lua_State *L)
{
	lua_newtable(L);

	STRUCT_SECTION_FOREACH(zbook_lua_binding, binding) {
		push_category(L, binding->category);

		lua_pushcfunction(L, binding->open);
		lua_call(L, 0, 1);
		lua_setfield(L, -2, binding->name);

		lua_pop(L, 1); /* category table */
	}

	return 1;
}

int zbook_lua_setup(lua_State *L)
{
	luaL_requiref(L, "zbook", luaopen_zbook, 1);
	lua_pop(L, 1);

	return 0;
}
