/*************************************************************************
 * @file zbook_lua_binding.h
 *
 * @brief Self-registration mechanism for zbook.<category>.<name> Lua bindings.
 *
 * Each binding (lua_zbook_led.c, lua_zbook_btn.c, ...) declares itself with
 * ZBOOK_LUA_BINDING_DEFINE(); luaopen_zbook() iterates the resulting Zephyr
 * iterable section to build the `zbook` table without any per-binding
 * #ifdef. Adding a new binding only touches its own .c file.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/
#ifndef ZBOOK_LUA_BINDING_H
#define ZBOOK_LUA_BINDING_H

#include <lua.h>
#include <zephyr/sys/iterable_sections.h>

/**
 * @brief One entry of the zbook.<category>.<name> registry.
 */
struct zbook_lua_binding {
	const char *category; /**< Category table under zbook (e.g. "actuators"). */
	const char *name;     /**< Field name under the category (e.g. "led"). */
	lua_CFunction open;   /**< Opener: pushes the binding table, returns 1. */
};

/**
 * @brief Register a binding under zbook.<_category>.<_name>.
 *
 * @param _category Unquoted category name, matching the interface category
 *                  (actuators, io, protocols, sensors, storage).
 * @param _name Unquoted peripheral name (e.g. led, btn).
 * @param _open luaopen_zbook_<name> function for this binding.
 */
#define ZBOOK_LUA_BINDING_DEFINE(_category, _name, _open)                                          \
	const STRUCT_SECTION_ITERABLE(zbook_lua_binding,                                           \
				      _zbook_lua_binding_##_category##_##_name) = {                \
		.category = #_category,                                                            \
		.name = #_name,                                                                    \
		.open = (_open),                                                                   \
	}

/**
 * @brief Name/value pair used to export enum constants to a binding table.
 */
struct zbook_lua_const {
	const char *name;  /**< Field name in the Lua table. */
	lua_Integer value; /**< Integer value. */
};

/**
 * @brief Set every constant of @p consts as an integer field of the table
 * at the top of the stack.
 *
 * @param L Lua state.
 * @param consts Array of constants.
 * @param count Number of entries in @p consts.
 */
static inline void zbook_lua_set_consts(lua_State *L, const struct zbook_lua_const *consts,
					size_t count)
{
	for (size_t i = 0; i < count; i++) {
		lua_pushinteger(L, consts[i].value);
		lua_setfield(L, -2, consts[i].name);
	}
}

#endif /* ZBOOK_LUA_BINDING_H */
