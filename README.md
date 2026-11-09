# fw-lua-zbook

ZBook firmware written in Lua: a memory game on the four onboard LEDs and
buttons. The board plays a sequence on LED0..LED3; repeat it with BTN0..BTN3
(BTNn answers LEDn). Each round adds one step and plays a little faster.

| State | What you see | What to do |
|---|---|---|
| Idle | all LEDs blink slowly | press any button to start |
| Playing | LEDs show the sequence | repeat it; each correct press flashes its LED |
| Game over (wrong button or 3 s without input) | all LEDs blink fast, score on the console | wait, it returns to idle |
| Abort | LEDs off | long-press any button (1.5 s) |

The whole application is [`src/game.lua`](src/game.lua). C, Kconfig and the
Lua runtime come from west modules:

- [`sdk-zbook-interface`](https://github.com/JoaoMatheusND/sdk-zbook-interface):
  C hardware abstraction (LED, buttons, ...), Zephyr and the board.
- [`sdk-lua-zbook`](https://github.com/JoaoMatheusND/sdk-lua-zbook/tree/zbook-lua-init):
  the `zbook` Lua table (`zbook.actuators.led`, `zbook.io.btn`, ...) and the
  [`lua_zephyr`](https://github.com/rodrigopex/lua_zephyr) runtime.

## Build

```sh
west init -m https://github.com/<you>/fw-lua-zbook.git zbook-ws
cd zbook-ws
west update
west sdk install --toolchains arm-zephyr-eabi   # once per machine
west build -b zbook@p2/rp2350b/m33 app
west flash
```

Console output (115200 baud):

```
zbook memory game: press any button to start
level 1
level 2
...
wrong: expected BTN1, got BTN2
game over, score 3
```

## Layout

```
app/                    # this repo (west manifest)
├── west.yml            # pins sdk-zbook-interface + sdk-lua-zbook
├── CMakeLists.txt      # declares the Lua thread(s)
├── prj.conf            # enables the Lua runtime + the bindings used
└── src/game.lua        # the firmware
```

- **Add a Lua thread:** another `zbook_lua_define_thread(src/<name>.lua)` in
  `CMakeLists.txt`. Each thread gets its own heap, stack and the global
  `zbook` table. Tune it with `CONFIG_<NAME>_LUA_THREAD_{STACK,HEAP}_SIZE`.
- **Use another peripheral:** enable its binding in `prj.conf`
  (`CONFIG_ZBOOK_LUA_<PERIPH>=y`); the binding selects the interface driver.
- **Standard libraries** (`math`, `string`, `table`, ...) are enabled with
  `CONFIG_LUA_LIB_<NAME>=y` and must be loaded with `require("<name>")`.
- **Editor completion:** `.luarc.json` points LuaLS at the binding stubs in
  `../wrapper/lua/sdk`.

## Updating the SDKs

`west.yml` pins both SDKs by commit. Bump the `revision:` and run
`west update`.
