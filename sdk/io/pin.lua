---@meta

--- ZBook header pin functions (zbook.io.pin), CONFIG_ZBOOK_LUA_PIN.
--- Wraps interface/includes/io/zbook_pin.h. One pin object per header pin:
--- set_function() on an already configured pin returns the same object.

---@class zbook.io.pin
---@field IO_01 integer
---@field IO_39 integer
---@field IO_45 integer # ADC capable
---@field IO_46 integer # ADC capable
---@field LOW integer
---@field HIGH integer
---@field OPEN_DRAIN integer
local pin = {}
zbook.io.pin = pin

---@alias zbook.pin.func "gpio"|"uart"|"pwm"|"adc"

---@class zbook.pin.cfg
---@field func zbook.pin.func
---@field dir? "in"|"out" # gpio, default "in"
---@field pull? "none"|"up"|"down" # gpio, default "none"
---@field state? integer # gpio output initial state, default pin.LOW
---@field freq? integer # pwm, required (Hz)
---@field duty? integer # pwm, 0-100, default 0
---@field polarity? "normal"|"inverted" # pwm, default "normal"
---@field resolution? integer # adc bits, default 12
---@field vref? number # adc reference voltage
---@field role? "tx"|"rx" # uart, required
---@field baud? integer # uart, required
---@field data_bits? integer # uart, 5-9, default 8
---@field parity? "none"|"odd"|"even" # uart, default "none"

---@class zbook.pin.handle
local handle = {}

---Assign a function to a header pin (tearing down the previous one).
---@param gpio integer # pin.IO_*
---@param cfg zbook.pin.cfg
---@return integer err
---@return zbook.pin.handle|nil p
function pin.set_function(gpio, cfg) end

---Whether the pin hardware can take a function.
---@param gpio integer
---@param func zbook.pin.func
---@return boolean
function pin.supports(gpio, func) end

---@param gpio integer
---@return integer err # -ENODATA when unconfigured
---@return zbook.pin.func|nil func
function pin.get_function(gpio) end

---gpio: state (0/1/2); pwm: duty 0-100; uart (tx): string of bytes.
---@param value integer|string
---@return integer err
function handle:write(value) end

---gpio: 0/1; adc: raw sample.
---@return integer err
---@return integer|nil value
function handle:read() end

---Return the pin to the unconfigured state.
---@return integer err
function handle:release() end

return pin
