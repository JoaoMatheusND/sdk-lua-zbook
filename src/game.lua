-- ZBook memory game.
--
-- The board shows a sequence on LED0..LED3; repeat it with BTN0..BTN3
-- (BTNn answers LEDn). Each round adds one step and plays a bit faster.
--
--   idle         all LEDs blink slowly; press any button to start
--   wrong/slow   all LEDs blink fast, score is printed, back to idle
--   long press   (any button, CONFIG_BTN_LONG_PRESS_MS) aborts the game
--   MAX_LEVEL    winning animation, back to idle

local zephyr = require("zephyr")
local math = require("math")
local led = zbook.actuators.led
local btn = zbook.io.btn

local MAX_LEVEL = 32
local INPUT_TIMEOUT_MS = 3000
local STEP_MS_START = 450
local STEP_MS_MIN = 150
local STEP_MS_DECAY = 20
local GAP_MS = 120
local FEEDBACK_MS = 150

local LEDS = { led.LED0, led.LED1, led.LED2, led.LED3 }

local OK, FAIL, ABORT = 1, 2, 3

local function check(err, what)
	if err ~= 0 then
		error(what .. " failed: " .. err)
	end
end

local function flash(l, ms)
	led.on(l)
	zephyr.msleep(ms)
	led.off(l)
end

local function flush_buttons()
	while btn.wait(0) == 0 do
	end
end

local function wait_start()
	led.blink_mask(LEDS, led.BLINK_SLOW)
	flush_buttons()

	local ticks = 0
	while true do
		local err, _, evt = btn.wait(10)
		if err == 0 and evt == btn.EVT_PRESSED then
			break
		end
		ticks = ticks + 1
	end

	led.off_mask(LEDS)
	zephyr.msleep(500)

	return ticks
end

local function step_ms(level)
	return math.max(STEP_MS_MIN, STEP_MS_START - (level - 1) * STEP_MS_DECAY)
end

local function play_sequence(seq, level)
	local on_ms = step_ms(level)

	for i = 1, level do
		flash(seq[i], on_ms)
		zephyr.msleep(GAP_MS)
	end
end

local function read_answer(seq, level)
	flush_buttons()

	for i = 1, level do
		local err, b, evt = btn.wait(INPUT_TIMEOUT_MS)

		if err ~= 0 then
			zephyr.printk("too slow")
			return FAIL
		end

		if evt == btn.EVT_LONG_PRESSED then
			return ABORT
		end

		if b ~= seq[i] then
			zephyr.printk("wrong: expected BTN" .. seq[i] .. ", got BTN" .. b)
			return FAIL
		end

		flash(LEDS[b + 1], FEEDBACK_MS)
	end

	return OK
end

local function game_over(score)
	zephyr.printk("game over, score " .. score)
	led.blink_mask(LEDS, led.BLINK_FAST)
	zephyr.msleep(2000)
	led.off_mask(LEDS)
end

local function victory()
	zephyr.printk("you win! all " .. MAX_LEVEL .. " levels")
	for _ = 1, 4 do
		for _, l in ipairs(LEDS) do
			flash(l, 80)
		end
	end
end

local function play()
	local seq = {}

	for level = 1, MAX_LEVEL do
		seq[level] = LEDS[math.random(#LEDS)]

		zephyr.printk("level " .. level)
		play_sequence(seq, level)

		local result = read_answer(seq, level)
		if result == ABORT then
			zephyr.printk("aborted")
			led.off_mask(LEDS)
			return
		elseif result == FAIL then
			game_over(level - 1)
			return
		end

		zephyr.msleep(600)
	end

	victory()
end

check(led.init(), "led.init")
check(btn.init(), "btn.init")

for b = btn.BTN0, btn.BTN3 do
	check(btn.enable(b, btn.EVT_PRESSED | btn.EVT_LONG_PRESSED), "btn.enable")
end

zephyr.printk("zbook memory game: press any button to start")

while true do
	math.randomseed(wait_start())
	play()
end
