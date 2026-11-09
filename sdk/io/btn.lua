---@meta

--- ZBook buttons (zbook.io.btn), CONFIG_ZBOOK_LUA_BTN.
--- Wraps interface/includes/io/zbook_btn.h. Events are delivered to the Lua
--- thread over the zbus channel `zbook_lua_btn_chan` (subscriber
--- `zbook_lua_btn_sub`); a single Lua thread should consume them.

---@class zbook.io.btn
---@field BTN0 integer
---@field BTN1 integer
---@field BTN2 integer
---@field BTN3 integer
---@field ALL integer # Every button.
---@field EVT_NONE integer
---@field EVT_PRESSED integer
---@field EVT_RELEASED integer
---@field EVT_LONG_PRESSED integer # Fires once after CONFIG_BTN_LONG_PRESS_MS, besides PRESSED/RELEASED.
---@field EVT_BOTH integer # PRESSED | RELEASED
---@field EVT_ALL integer
local btn = {}
zbook.io.btn = btn

---Check the button input device is ready.
---@return integer err
function btn.init() end

---Start forwarding events of a button to btn.wait().
---@param b integer # btn.BTNn or btn.ALL
---@param evt_mask? integer # OR of btn.EVT_* (default btn.EVT_ALL)
---@return integer err
function btn.enable(b, evt_mask) end

---Stop forwarding events of a button.
---@param b integer
---@return integer err
function btn.disable(b) end

---Block until a button event arrives.
---@param timeout_ms? integer # Negative (default) waits forever.
---@return integer err # 0, or negative errno (-EAGAIN on timeout).
---@return integer|nil b # Button that fired.
---@return integer|nil evt # Single btn.EVT_* value.
function btn.wait(timeout_ms) end

return btn
