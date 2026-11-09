/*************************************************************************
 * @file lua_zbook_led.c
 *
 * @brief Lua binding for the Zbook LED actuator (zbook.actuators.led).
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifdef CONFIG_ZBOOK_LUA_LED

#include "zbook_lua_binding.h"

#include <actuators/zbook_led.h>

#include <lauxlib.h>
#include <zephyr/sys/util.h>

/**
 * @brief Read a LED bitmask argument: either an integer mask or an array of
 * LED indexes ({led.LED0, led.LED2}).
 */
static uint32_t check_mask(lua_State *L, int idx)
{
	if (!lua_istable(L, idx)) {
		return (uint32_t)luaL_checkinteger(L, idx);
	}

	uint32_t mask = 0;
	lua_Integer len = luaL_len(L, idx);

	for (lua_Integer i = 1; i <= len; i++) {
		lua_geti(L, idx, i);
		lua_Integer led = luaL_checkinteger(L, -1);

		luaL_argcheck(L, led >= 0 && led < 32, idx, "LED index out of range");
		mask |= BIT(led);
		lua_pop(L, 1);
	}

	return mask;
}

/** @brief led.init() -> err */
static int l_led_init(lua_State *L)
{
	lua_pushinteger(L, zbook_led_init());

	return 1;
}

/** @brief led.on(led) -> err */
static int l_led_on(lua_State *L)
{
	lua_pushinteger(L, zbook_led_on((enum zbook_led)luaL_checkinteger(L, 1)));

	return 1;
}

/** @brief led.off(led) -> err */
static int l_led_off(lua_State *L)
{
	lua_pushinteger(L, zbook_led_off((enum zbook_led)luaL_checkinteger(L, 1)));

	return 1;
}

/** @brief led.toggle(led) -> err */
static int l_led_toggle(lua_State *L)
{
	lua_pushinteger(L, zbook_led_toggle((enum zbook_led)luaL_checkinteger(L, 1)));

	return 1;
}

/** @brief led.blink(led, state) -> err */
static int l_led_blink(lua_State *L)
{
	enum zbook_led led = (enum zbook_led)luaL_checkinteger(L, 1);
	enum zbook_led_state state =
		(enum zbook_led_state)luaL_optinteger(L, 2, ZBOOK_LED_BLINK_DEFAULT);

	lua_pushinteger(L, zbook_led_blink(led, state));

	return 1;
}

/** @brief led.on_mask(mask) -> err */
static int l_led_on_mask(lua_State *L)
{
	lua_pushinteger(L, zbook_led_on_mask(check_mask(L, 1)));

	return 1;
}

/** @brief led.off_mask(mask) -> err */
static int l_led_off_mask(lua_State *L)
{
	lua_pushinteger(L, zbook_led_off_mask(check_mask(L, 1)));

	return 1;
}

/** @brief led.toggle_mask(mask) -> err */
static int l_led_toggle_mask(lua_State *L)
{
	lua_pushinteger(L, zbook_led_toggle_mask(check_mask(L, 1)));

	return 1;
}

/** @brief led.blink_mask(mask, state) -> err */
static int l_led_blink_mask(lua_State *L)
{
	uint32_t mask = check_mask(L, 1);
	enum zbook_led_state state =
		(enum zbook_led_state)luaL_optinteger(L, 2, ZBOOK_LED_BLINK_DEFAULT);

	lua_pushinteger(L, zbook_led_blink_mask(mask, state));

	return 1;
}

static const luaL_Reg led_funcs[] = {
	{"init", l_led_init},
	{"on", l_led_on},
	{"off", l_led_off},
	{"toggle", l_led_toggle},
	{"blink", l_led_blink},
	{"on_mask", l_led_on_mask},
	{"off_mask", l_led_off_mask},
	{"toggle_mask", l_led_toggle_mask},
	{"blink_mask", l_led_blink_mask},
	{NULL, NULL},
};

static const struct zbook_lua_const led_consts[] = {
	{"LED0", ZBOOK_LED_0},
	{"LED1", ZBOOK_LED_1},
	{"LED2", ZBOOK_LED_2},
	{"LED3", ZBOOK_LED_3},
	{"ALL", ZBOOK_LED_ALL},
	{"OFF", ZBOOK_LED_OFF},
	{"ON", ZBOOK_LED_ON},
	{"BLINK_DEFAULT", ZBOOK_LED_BLINK_DEFAULT},
	{"BLINK_SLOW", ZBOOK_LED_BLINK_SLOW},
	{"BLINK_FAST", ZBOOK_LED_BLINK_FAST},
};

static int luaopen_zbook_led(lua_State *L)
{
	luaL_newlib(L, led_funcs);
	zbook_lua_set_consts(L, led_consts, ARRAY_SIZE(led_consts));

	return 1;
}

ZBOOK_LUA_BINDING_DEFINE(actuators, led, luaopen_zbook_led);

#endif /* CONFIG_ZBOOK_LUA_LED */
