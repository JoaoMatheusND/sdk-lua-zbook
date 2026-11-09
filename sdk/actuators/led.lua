---@meta

--- ZBook LED actuator (zbook.actuators.led), CONFIG_ZBOOK_LUA_LED.
--- Wraps interface/includes/actuators/zbook_led.h. Every function returns
--- 0 on success or a negative errno.

---@class zbook.actuators.led
---@field LED0 integer
---@field LED1 integer
---@field LED2 integer
---@field LED3 integer
---@field ALL integer # Every LED at once.
---@field OFF integer
---@field ON integer
---@field BLINK_DEFAULT integer
---@field BLINK_SLOW integer
---@field BLINK_FAST integer
local led = {}
zbook.actuators.led = led

---@alias zbook.led.mask integer|integer[] # Bitmask (1 << led.LEDn) or array of LED indexes.

---Check every LED is ready to use.
---@return integer err
function led.init() end

---@param l integer # led.LEDn or led.ALL
---@return integer err
function led.on(l) end

---@param l integer
---@return integer err
function led.off(l) end

---Toggle a LED (stops an ongoing blink first).
---@param l integer
---@return integer err
function led.toggle(l) end

---@param l integer
---@param state? integer # led.BLINK_DEFAULT (default), led.BLINK_SLOW or led.BLINK_FAST
---@return integer err
function led.blink(l, state) end

---Not atomic: LEDs processed before a failure keep their new state.
---@param mask zbook.led.mask
---@return integer err
function led.on_mask(mask) end

---@param mask zbook.led.mask
---@return integer err
function led.off_mask(mask) end

---@param mask zbook.led.mask
---@return integer err
function led.toggle_mask(mask) end

---@param mask zbook.led.mask
---@param state? integer
---@return integer err
function led.blink_mask(mask, state) end

return led
