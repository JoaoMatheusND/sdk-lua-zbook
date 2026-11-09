/*************************************************************************
 * @file lua_zbook_pin.c
 *
 * @brief Lua binding for the Zbook header pin-function factory (zbook.io.pin).
 *
 * The interface keeps one static struct zbook_pin per header pin, so this
 * binding keeps exactly one Lua userdata per pin, cached in the registry:
 * set_function() on an already-wrapped pin returns the same object, and no
 * stale wrapper can release a pin another wrapper is still using.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifdef CONFIG_ZBOOK_LUA_PIN

#include "zbook_lua_binding.h"

#include <io/zbook_pin.h>

#include <errno.h>
#include <lauxlib.h>
#include <string.h>

#define PIN_METATABLE "zbook.io.pin"
#define PIN_CACHE     "zbook.io.pin.cache"

/** @brief Lua userdata wrapping a static interface pin handle. */
struct lua_zbook_pin {
	struct zbook_pin *handle; /**< Interface handle (static, never freed). */
};

/* Order must match enum zbook_pin_header_function. */
static const char *const func_names[] = {"gpio", "uart", "pwm", "adc", NULL};

/* Order must match the respective interface enums. */
static const char *const dir_names[] = {"in", "out", NULL};
static const char *const pull_names[] = {"none", "up", "down", NULL};
static const char *const role_names[] = {"tx", "rx", NULL};
static const char *const polarity_names[] = {"normal", "inverted", NULL};
/* Zephyr uart_config_parity order (enum zbook_uart_parity is not defined yet). */
static const char *const parity_names[] = {"none", "odd", "even", NULL};

/** @brief Read cfg[key] as the index of one of @p names; @p def when absent
 * (NULL makes the field required). */
static int opt_option(lua_State *L, int cfg, const char *key, const char *def,
		      const char *const names[])
{
	lua_getfield(L, cfg, key);
	const char *value = lua_isnil(L, -1) ? def : lua_tostring(L, -1);

	if (value == NULL) {
		return luaL_error(L, "pin config: '%s' is required", key);
	}

	for (int i = 0; names[i] != NULL; i++) {
		if (strcmp(names[i], value) == 0) {
			lua_pop(L, 1);
			return i;
		}
	}

	return luaL_error(L, "pin config: invalid %s '%s'", key, value);
}

/** @brief Read cfg[key] as an integer, @p def when absent. */
static lua_Integer opt_integer(lua_State *L, int cfg, const char *key, lua_Integer def)
{
	lua_getfield(L, cfg, key);
	lua_Integer value = luaL_optinteger(L, -1, def);

	lua_pop(L, 1);

	return value;
}

/** @brief Read cfg[key] as a required integer. */
static lua_Integer check_integer(lua_State *L, int cfg, const char *key)
{
	lua_getfield(L, cfg, key);
	if (!lua_isinteger(L, -1)) {
		return luaL_error(L, "pin config: '%s' must be an integer", key);
	}

	lua_Integer value = lua_tointeger(L, -1);

	lua_pop(L, 1);

	return value;
}

/** @brief Read cfg[key] as a number, @p def when absent. */
static lua_Number opt_number(lua_State *L, int cfg, const char *key, lua_Number def)
{
	lua_getfield(L, cfg, key);
	lua_Number value = luaL_optnumber(L, -1, def);

	lua_pop(L, 1);

	return value;
}

/**
 * @brief Convert a Lua config table into struct zbook_pin_header_config.
 *
 * { func = "gpio", dir = "in"|"out", pull = "none"|"up"|"down", state = 0|1|2 }
 * { func = "pwm",  freq = <Hz>, duty = 0..100, polarity = "normal"|"inverted" }
 * { func = "adc",  resolution = <bits>, vref = <volts> }
 * { func = "uart", role = "tx"|"rx", baud = <bps>, data_bits = 5..9,
 *   parity = "none"|"odd"|"even" }
 */
static void check_cfg(lua_State *L, int cfg, struct zbook_pin_header_config *out)
{
	luaL_checktype(L, cfg, LUA_TTABLE);

	*out = (struct zbook_pin_header_config){0};
	out->function =
		(enum zbook_pin_header_function)opt_option(L, cfg, "func", NULL, func_names);

	switch (out->function) {
	case ZBOOK_PIN_HEADER_FUNCTION_GPIO:
		out->cfg.gpio.direction =
			(enum zbook_pin_header_direction)opt_option(L, cfg, "dir", "in", dir_names);
		out->cfg.gpio.pull =
			(enum zbook_pin_header_pull)opt_option(L, cfg, "pull", "none", pull_names);
		out->cfg.gpio.state = (enum zbook_pin_header_state)opt_integer(
			L, cfg, "state", ZBOOK_PIN_HEADER_STATE_LOW);
		break;
	case ZBOOK_PIN_HEADER_FUNCTION_PWM:
		out->cfg.pwm.frequency = (uint32_t)check_integer(L, cfg, "freq");
		out->cfg.pwm.duty_cycle = (uint8_t)opt_integer(L, cfg, "duty", 0);
		out->cfg.pwm.polarity = (enum zbook_pin_header_pwm_polarity)opt_option(
			L, cfg, "polarity", "normal", polarity_names);
		break;
	case ZBOOK_PIN_HEADER_FUNCTION_ADC:
		out->cfg.adc.resolution = (uint8_t)opt_integer(L, cfg, "resolution", 12);
		out->cfg.adc.reference_voltage = (float)opt_number(L, cfg, "vref", 0);
		break;
	case ZBOOK_PIN_HEADER_FUNCTION_UART:
		out->cfg.uart.role = (enum zbook_pin_header_uart_role)opt_option(L, cfg, "role",
										 NULL, role_names);
		out->cfg.uart.baudrate = (uint32_t)check_integer(L, cfg, "baud");
		out->cfg.uart.data_bits = (uint8_t)opt_integer(L, cfg, "data_bits", 0);
		out->cfg.uart.parity = (uint8_t)opt_option(L, cfg, "parity", "none", parity_names);
		break;
	default:
		break;
	}
}

/** @brief Validate and return the pin userdata at @p idx. */
static struct lua_zbook_pin *check_pin(lua_State *L, int idx)
{
	return luaL_checkudata(L, idx, PIN_METATABLE);
}

/** @brief Push the cached userdata for @p handle, creating it on first use. */
static void push_pin(lua_State *L, struct zbook_pin *handle)
{
	lua_getfield(L, LUA_REGISTRYINDEX, PIN_CACHE);

	if (lua_rawgeti(L, -1, handle->gpio) == LUA_TUSERDATA) {
		lua_remove(L, -2);
		return;
	}

	lua_pop(L, 1);

	struct lua_zbook_pin *pin = lua_newuserdatauv(L, sizeof(*pin), 0);

	pin->handle = handle;
	luaL_setmetatable(L, PIN_METATABLE);

	lua_pushvalue(L, -1);
	lua_rawseti(L, -3, handle->gpio);
	lua_remove(L, -2);
}

/** @brief pin.set_function(gpio, cfg) -> err, pin|nil */
static int l_pin_set_function(lua_State *L)
{
	enum zbook_pin_header gpio = (enum zbook_pin_header)luaL_checkinteger(L, 1);
	struct zbook_pin_header_config cfg;
	struct zbook_pin *handle = NULL;

	check_cfg(L, 2, &cfg);

	int err = zbook_pin_set_function(gpio, &cfg, &handle);

	lua_pushinteger(L, err);

	if (err == 0 && handle != NULL) {
		push_pin(L, handle);
	} else {
		lua_pushnil(L);
	}

	return 2;
}

/** @brief pin.supports(gpio, func) -> boolean */
static int l_pin_supports(lua_State *L)
{
	enum zbook_pin_header gpio = (enum zbook_pin_header)luaL_checkinteger(L, 1);
	int func = luaL_checkoption(L, 2, NULL, func_names);

	lua_pushboolean(L, zbook_pin_supports(gpio, (enum zbook_pin_header_function)func));

	return 1;
}

/** @brief pin.get_function(gpio) -> err, func|nil */
static int l_pin_get_function(lua_State *L)
{
	enum zbook_pin_header gpio = (enum zbook_pin_header)luaL_checkinteger(L, 1);
	enum zbook_pin_header_function func;
	int err = zbook_pin_get_function(gpio, &func);

	lua_pushinteger(L, err);

	if (err == 0 && (size_t)func < ZBOOK_PIN_HEADER_FUNCTION_AMOUNT) {
		lua_pushstring(L, func_names[func]);
	} else {
		lua_pushnil(L);
	}

	return 2;
}

/**
 * @brief p:write(value) -> err
 *
 * - gpio: value is the state (0 = low, 1 = high, 2 = open drain).
 * - pwm:  value is the duty cycle in percent (0-100).
 * - uart: value is a string of bytes to transmit.
 */
static int l_pin_write(lua_State *L)
{
	struct lua_zbook_pin *pin = check_pin(L, 1);
	int err;

	if (lua_type(L, 2) == LUA_TSTRING) {
		size_t len;
		const char *data = lua_tolstring(L, 2, &len);

		err = zbook_pin_write(pin->handle, data, len);
	} else {
		uint8_t value = (uint8_t)luaL_checkinteger(L, 2);

		err = zbook_pin_write(pin->handle, &value, sizeof(value));
	}

	lua_pushinteger(L, err);

	return 1;
}

/**
 * @brief p:read() -> err, value|nil
 *
 * - gpio: value is 0 (low) or 1 (high).
 * - adc:  value is one raw sample at the configured resolution.
 */
static int l_pin_read(lua_State *L)
{
	struct lua_zbook_pin *pin = check_pin(L, 1);
	lua_Integer value = 0;
	int err;

	if (pin->handle->function == ZBOOK_PIN_HEADER_FUNCTION_ADC) {
		uint16_t sample = 0;

		err = zbook_pin_read(pin->handle, &sample, sizeof(sample));
		value = sample;
	} else {
		uint8_t state = 0;

		err = zbook_pin_read(pin->handle, &state, sizeof(state));
		value = state;
	}

	lua_pushinteger(L, err);

	if (err == 0) {
		lua_pushinteger(L, value);
	} else {
		lua_pushnil(L);
	}

	return 2;
}

/** @brief p:release() -> err */
static int l_pin_release(lua_State *L)
{
	lua_pushinteger(L, zbook_pin_release(check_pin(L, 1)->handle));

	return 1;
}

/** @brief tostring(p) */
static int l_pin_tostring(lua_State *L)
{
	struct lua_zbook_pin *pin = check_pin(L, 1);

	lua_pushfstring(L, "zbook.io.pin { gpio=%d }", (int)pin->handle->gpio);

	return 1;
}

static const luaL_Reg pin_funcs[] = {
	{"set_function", l_pin_set_function},
	{"supports", l_pin_supports},
	{"get_function", l_pin_get_function},
	{NULL, NULL},
};

static const luaL_Reg pin_methods[] = {
	{"write", l_pin_write},
	{"read", l_pin_read},
	{"release", l_pin_release},
	{NULL, NULL},
};

static const struct zbook_lua_const pin_consts[] = {
	{"IO_01", ZBOOK_PIN_HEADER_IO_01},
	{"IO_39", ZBOOK_PIN_HEADER_IO_39},
	{"IO_45", ZBOOK_PIN_HEADER_IO_45},
	{"IO_46", ZBOOK_PIN_HEADER_IO_46},
	{"LOW", ZBOOK_PIN_HEADER_STATE_LOW},
	{"HIGH", ZBOOK_PIN_HEADER_STATE_HIGH},
	{"OPEN_DRAIN", ZBOOK_PIN_HEADER_STATE_OPEN_DRAIN},
};

static int luaopen_zbook_pin(lua_State *L)
{
	if (luaL_newmetatable(L, PIN_METATABLE)) {
		luaL_newlib(L, pin_methods);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, l_pin_tostring);
		lua_setfield(L, -2, "__tostring");
	}
	lua_pop(L, 1);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, PIN_CACHE);

	luaL_newlib(L, pin_funcs);
	zbook_lua_set_consts(L, pin_consts, ARRAY_SIZE(pin_consts));

	return 1;
}

ZBOOK_LUA_BINDING_DEFINE(io, pin, luaopen_zbook_pin);

#endif /* CONFIG_ZBOOK_LUA_PIN */
