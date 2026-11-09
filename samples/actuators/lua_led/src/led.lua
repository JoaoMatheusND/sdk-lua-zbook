-- Walk a lit LED across LED0..LED3, then blink them all with each pattern.
local zephyr = require("zephyr")
local led = zbook.actuators.led

local err = led.init()
if err ~= 0 then
	zephyr.log_err("led.init failed: " .. err)
	return
end

local leds = { led.LED0, led.LED1, led.LED2, led.LED3 }

for _ = 1, 3 do
	for _, l in ipairs(leds) do
		led.on(l)
		zephyr.msleep(200)
		led.off(l)
	end
end

for _, pattern in ipairs({ led.BLINK_SLOW, led.BLINK_DEFAULT, led.BLINK_FAST }) do
	led.blink_mask(leds, pattern)
	zephyr.msleep(3000)
end

led.off_mask(leds)
zephyr.printk("lua_led done")
