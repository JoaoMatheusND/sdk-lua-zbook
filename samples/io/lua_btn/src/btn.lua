-- BTNn press toggles LEDn; long-press on any button turns every LED off.
local zephyr = require("zephyr")
local btn = zbook.io.btn
local led = zbook.actuators.led

assert(led.init() == 0, "led.init failed")
assert(btn.init() == 0, "btn.init failed")

for b = btn.BTN0, btn.BTN3 do
	assert(btn.enable(b, btn.EVT_PRESSED | btn.EVT_LONG_PRESSED) == 0)
end

zephyr.printk("lua_btn ready: press a button")

while true do
	local err, b, evt = btn.wait(-1)
	if err == 0 then
		if evt == btn.EVT_LONG_PRESSED then
			led.off(led.ALL)
			zephyr.printk("BTN" .. b .. " long press: all LEDs off")
		else
			led.toggle(b)
			zephyr.printk("BTN" .. b .. " pressed: toggle LED" .. b)
		end
	end
end
