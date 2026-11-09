-- Smallest zbook Lua app: list every binding compiled into this firmware.
local zephyr = require("zephyr")
local zbook = require("zbook")

zephyr.printk("zbook Lua binding up")

for category, bindings in pairs(zbook) do
	for name in pairs(bindings) do
		zephyr.printk("  zbook." .. category .. "." .. name)
	end
end
