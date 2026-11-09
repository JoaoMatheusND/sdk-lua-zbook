/*************************************************************************
 * @file lua_zbook_btn.c
 *
 * @brief Lua binding for the Zbook buttons (zbook.io.btn).
 *
 * Button callbacks run outside the Lua thread (input thread / system
 * workqueue), so they never touch a lua_State: each event is published on
 * the zbus channel zbook_lua_btn_chan and the Lua thread consumes it through
 * the message subscriber zbook_lua_btn_sub, either with btn.wait() or
 * directly via zephyr.zbus.observer_declare("zbook_lua_btn_sub").
 *
 * @note Single consumer: one Lua thread should wait on button events.
 *
 * @author João Matheus Nascimento Dias (joao.dias@edge.ufal.br)
 * @version 0.1
 * @date 09/11/2026
 *
 * @copyright Centro de Inovação EDGE 2026. Todos os direitos reservados.
 *
 ***********************************************************************/

#ifdef CONFIG_ZBOOK_LUA_BTN

#include "zbook_lua_binding.h"

#include <io/zbook_btn.h>

#include <lauxlib.h>
#include <luaz_msg_descr.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

/**
 * @brief Message published on zbook_lua_btn_chan for every button event.
 */
struct zbook_lua_btn_msg {
	uint8_t btn; /**< enum zbook_btn */
	uint8_t evt; /**< enum zbook_btn_evt (a single event, never a mask) */
};

static const struct lua_msg_field_descr btn_msg_fields[] = {
	LUA_MSG_FIELD(struct zbook_lua_btn_msg, btn, LUA_MSG_TYPE_UINT),
	LUA_MSG_FIELD(struct zbook_lua_btn_msg, evt, LUA_MSG_TYPE_UINT),
};

ZBUS_MSG_SUBSCRIBER_DEFINE(zbook_lua_btn_sub);

ZBUS_CHAN_DEFINE(zbook_lua_btn_chan, struct zbook_lua_btn_msg, NULL,
		 LUA_ZBUS_MSG_DESCR(struct zbook_lua_btn_msg, btn_msg_fields),
		 ZBUS_OBSERVERS(zbook_lua_btn_sub), ZBUS_MSG_INIT(.btn = 0, .evt = 0));

/** @brief Interface callback: forward the event to the Lua thread via zbus. */
static void btn_to_zbus(enum zbook_btn btn, enum zbook_btn_evt evt, void *user_data)
{
	ARG_UNUSED(user_data);

	const struct zbook_lua_btn_msg msg = {
		.btn = (uint8_t)btn,
		.evt = (uint8_t)evt,
	};

	(void)zbus_chan_pub(&zbook_lua_btn_chan, &msg,
			    K_MSEC(CONFIG_ZBOOK_LUA_BTN_QUEUE_TIMEOUT_MS));
}

/** @brief Convert a Lua timeout in ms (negative = forever) to k_timeout_t. */
static k_timeout_t to_timeout(lua_Integer ms)
{
	return ms < 0 ? K_FOREVER : K_MSEC(ms);
}

/** @brief btn.init() -> err */
static int l_btn_init(lua_State *L)
{
	lua_pushinteger(L, zbook_btn_init());

	return 1;
}

/** @brief btn.enable(btn [, evt_mask = btn.EVT_ALL]) -> err */
static int l_btn_enable(lua_State *L)
{
	enum zbook_btn btn = (enum zbook_btn)luaL_checkinteger(L, 1);
	enum zbook_btn_evt evt = (enum zbook_btn_evt)luaL_optinteger(L, 2, ZBOOK_BTN_EVT_ALL);

	lua_pushinteger(L, zbook_btn_reg_cb(btn, evt, btn_to_zbus, NULL));

	return 1;
}

/** @brief btn.disable(btn) -> err */
static int l_btn_disable(lua_State *L)
{
	lua_pushinteger(L, zbook_btn_rm_cb((enum zbook_btn)luaL_checkinteger(L, 1)));

	return 1;
}

/**
 * @brief btn.wait([timeout_ms = -1]) -> err, btn, evt
 *
 * Blocks the calling Lua thread until a button event arrives. On timeout or
 * error, btn and evt are nil and err is negative (-EAGAIN on timeout).
 */
static int l_btn_wait(lua_State *L)
{
	const struct zbus_channel *chan;
	struct zbook_lua_btn_msg msg;
	int err = zbus_sub_wait_msg(&zbook_lua_btn_sub, &chan, &msg,
				    to_timeout(luaL_optinteger(L, 1, -1)));

	lua_pushinteger(L, err);

	if (err != 0 || chan != &zbook_lua_btn_chan) {
		lua_pushnil(L);
		lua_pushnil(L);
	} else {
		lua_pushinteger(L, msg.btn);
		lua_pushinteger(L, msg.evt);
	}

	return 3;
}

static const luaL_Reg btn_funcs[] = {
	{"init", l_btn_init},
	{"enable", l_btn_enable},
	{"disable", l_btn_disable},
	{"wait", l_btn_wait},
	{NULL, NULL},
};

static const struct zbook_lua_const btn_consts[] = {
	{"BTN0", ZBOOK_BTN_0},
	{"BTN1", ZBOOK_BTN_1},
	{"BTN2", ZBOOK_BTN_2},
	{"BTN3", ZBOOK_BTN_3},
	{"ALL", ZBOOK_BTN_ALL},
	{"EVT_NONE", ZBOOK_BTN_EVT_NONE},
	{"EVT_PRESSED", ZBOOK_BTN_EVT_PRESSED},
	{"EVT_RELEASED", ZBOOK_BTN_EVT_RELEASED},
	{"EVT_LONG_PRESSED", ZBOOK_BTN_EVT_LONG_PRESSED},
	{"EVT_BOTH", ZBOOK_BTN_EVT_BOTH},
	{"EVT_ALL", ZBOOK_BTN_EVT_ALL},
};

static int luaopen_zbook_btn(lua_State *L)
{
	luaL_newlib(L, btn_funcs);
	zbook_lua_set_consts(L, btn_consts, ARRAY_SIZE(btn_consts));

	return 1;
}

ZBOOK_LUA_BINDING_DEFINE(io, btn, luaopen_zbook_btn);

#endif /* CONFIG_ZBOOK_LUA_BTN */
