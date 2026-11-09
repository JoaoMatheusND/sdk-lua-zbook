# Planner — sdk-lua-zbook

Plano de construção do wrapper Lua para a `sdk-zbook-interface`, como módulo
Zephyr independente, adicionado via `west.yml` da interface no path
`wrapper/lua`.

Base: versão anterior em
[`JoaoMatheusND/zbook@develop/wrappers/lua`](https://github.com/JoaoMatheusND/zbook/tree/develop/wrappers/lua)
e runtime [`rodrigopex/lua_zephyr`](https://github.com/rodrigopex/lua_zephyr).

## Princípios

- **Interface não sabe que o wrapper existe.** Nenhum `#include <lua.h>`, nenhum
  símbolo Kconfig com nome de linguagem do lado da interface. O wrapper é quem
  `select`a os periféricos de que precisa.
- **Runtime Lua não é responsabilidade deste repo.** Thread, heap isolado, REPL,
  filesystem, bytecode e zbus vêm do `lua_zephyr`, que **não é modificado**.
  Integração só via hook fraco `<script>_lua_setup(L)`.
- **Adicionar periférico toca só o próprio `.c`** (+ stub `sdk/` + Kconfig),
  via registro em iterable section.
- **Callbacks nunca tocam o `lua_State` fora da thread Lua.** Eventos atravessam
  via zbus.

## O que a versão anterior já resolve

| Peça                                                 | Como era                                                    | Decisão                              |
| ----------------------------------------------------- | ----------------------------------------------------------- | ------------------------------------- |
| Runtime Lua (5.5.0, thread, heap, REPL, fs, bytecode) | módulo `lua_zephyr`                                      | ✅ manter; não vendorizar Lua        |
| `luaopen_zbook()` raiz                              | `zbook_lua.c`                                             | ✅ manter                             |
| Auto-registro de binding                              | `ZBOOK_LUA_PROTOCOL_DEFINE` + iterable section + `.ld`  | ✅ generalizar p/ todas as categorias |
| Hook `<script>_lua_setup()`                         | `zbook_lua_setup.c` escrito à mão em cada sample        | 🔁 gerar automaticamente              |
| Stubs LuaLS (`sdk/*.lua`, `.luarc.json`)          | um por periférico                                          | ✅ manter                             |
| Integração no build                                 | sample faz glob de `interface/src` e `wrappers/lua/src` | ❌ virar módulo west                 |
| Kconfig                                               | `depends on ZBOOK_SPI && LUA`                             | 🔁 trocar por `select`              |
| Convenção de retorno                                | `err` inteiro (`0` / `-errno`)                        | ❓ em aberto                          |

## 1. Topologia west

```
zb_workspace/
├── sdk/                         # sdk-zbook-interface (manifest/self)
├── zbook/                       # board module
├── deps/
│   ├── zephyr/
│   └── modules/
│       ├── hal/ ...
│       └── lua/lua_zephyr/      # puxado transitivamente pelo sdk-lua-zbook
└── wrapper/
    └── lua/                     # sdk-lua-zbook
```

`west.yml` da interface:

```yaml
- name: sdk-lua-zbook
  remote: zephyr-book
  revision: main
  path: wrapper/lua
  import: true
```

`west.yml` do sdk-lua-zbook (importado em cascata):

```yaml
manifest:
  remotes:
    - name: rodrigopex
      url-base: https://github.com/rodrigopex
  projects:
    - name: lua_zephyr
      remote: rodrigopex
      revision: 66fc52f          # pinar commit: repo é alpha
      submodules: true           # lua/ é submódulo (lua/lua.git)
      path: deps/modules/lua/lua_zephyr
```

`zephyr/module.yml`:

```yaml
name: zbook-lua
build:
  cmake: .
  kconfig: Kconfig
  depends:
    - lua               # nome do módulo do lua_zephyr
    - zbook-interface
```

Os três módulos (interface, lua_zephyr, wrapper) passam a ser descobertos
pelo west — sem `ZEPHYR_EXTRA_MODULES`, sem glob manual da interface.

> ⚠ Requer PR na interface adicionando `name: zbook-interface` no
> `zephyr/module.yml` (hoje o nome é derivado do diretório: `sdk`).

## 2. Estrutura do repo

```
sdk-lua-zbook/
├── west.yml
├── zephyr/module.yml
├── CMakeLists.txt                # zephyr_library + glob src/**; zephyr_linker_sources(.ld)
├── Kconfig
├── cmake/zbook_lua.cmake         # helper p/ apps/samples (ver §4)
├── templates/zbook_lua_setup.c.in
├── zbook_lua_binding.ld
├── include/
│   ├── zbook_lua.h               # int luaopen_zbook(lua_State *L)
│   └── zbook_lua_binding.h       # ZBOOK_LUA_BINDING_DEFINE(category, name, open)
├── src/
│   ├── zbook_lua.c               # monta zbook.<category>.<name> iterando a section
│   ├── actuators/
│   │   └── lua_zbook_led.c
│   └── io/
│       ├── lua_zbook_btn.c
│       └── lua_zbook_pin.c
├── sdk/                          # stubs LuaLS (---@meta), espelho 1:1 dos bindings
│   ├── zbook.lua
│   ├── actuators/led.lua
│   └── io/btn.lua, io/pin.lua
├── .luarc.json                   # workspace.library: sdk + stubs do lua_zephyr
├── samples/
│   ├── hello/
│   ├── actuators/lua_led/
│   └── io/lua_btn/, io/lua_pin/
└── tests/
```

Convenção de nomes herdada: `lua_zbook_<periph>.c/h` para bindings,
`zbook_lua.c/h` para o agregador. Cada `.c` de binding inteiro dentro de
`#ifdef CONFIG_ZBOOK_LUA_<PERIPH>` — CMake faz glob uma vez e nunca muda.

### Registro genérico por categoria

A versão anterior só tinha section para `protocols`. Generalizar:

```c
struct zbook_lua_binding {
	const char *category;   /* "actuators", "io", "protocols", "sensors", "storage" */
	const char *name;       /* "led", "btn", "pin", ... */
	lua_CFunction open;     /* luaopen_zbook_<periph> */
};

#define ZBOOK_LUA_BINDING_DEFINE(_cat, _name, _open)                                \
	static STRUCT_SECTION_ITERABLE(zbook_lua_binding, _zbook_lua_##_cat##_##_name) = { \
		.category = #_cat,                                                         \
		.name = #_name,                                                            \
		.open = (_open),                                                           \
	}
```

`luaopen_zbook()` itera a section e cria `zbook.<category>` sob demanda.

## 3. Kconfig

```kconfig
menuconfig ZBOOK_LUA
	bool "ZBook Lua binding"
	depends on LUA

if ZBOOK_LUA

config ZBOOK_LUA_LED
	bool "zbook.actuators.led"
	default y
	select ZBOOK_INTERFACE_ACTUATORS
	select ZBOOK_INTERFACE_ACTUATORS_LED

config ZBOOK_LUA_BTN
	bool "zbook.io.btn"
	default y
	select ZBOOK_INTERFACE_IO
	select ZBOOK_INTERFACE_IO_BTN
	select ZBUS

config ZBOOK_LUA_PIN
	bool "zbook.io.pin"
	default y
	select ZBOOK_INTERFACE_IO
	select ZBOOK_INTERFACE_IO_PINS

endif
```

> ⚠ Na interface os símbolos filhos têm `depends on` do pai (ex.:
> `ZBOOK_INTERFACE_ACTUATORS_LED` depende de `ZBOOK_INTERFACE_ACTUATORS`).
> `select` não satisfaz `depends` → warning e símbolo desligado. Por isso o
> wrapper seleciona pai **e** filho. Alternativa: ajustar a hierarquia na
> interface (o README dela descreve símbolos planos `ZBOOK_<PERIPH>`, que não
> batem com o código atual).

## 4. Helper CMake — fim do `zbook_lua_setup.c` manual

Problemas atuais em cada sample:

- `include($ENV{ZEPHYR_BASE}/modules/lua_zephyr/luaz.cmake)` — path frágil;
- um `.c` com `<script>_lua_setup()` escrito à mão.

`cmake/zbook_lua.cmake`:

- localiza `luaz.cmake` pelo path do west
  (`${CMAKE_CURRENT_LIST_DIR}/../../../deps/modules/lua/lua_zephyr`) ou
  `west list -f {abspath} lua_zephyr`;
- `zbook_lua_define_thread(<script>.lua)` → chama
  `luaz_define_source_thread()` e guarda o nome (pré-`find_package`);
- `zbook_lua_generate()` (pós-`find_package`) → `configure_file()` do
  template gerando `<nome>_lua_setup()` com
  `luaL_requiref(L, "zbook", luaopen_zbook, 1)`, depois
  `luaz_generate_threads()`.

Sample resultante:

```cmake
cmake_minimum_required(VERSION 3.20.0)
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/zbook_lua.cmake)
zbook_lua_define_thread(src/led.lua)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lua_led)
zbook_lua_generate()
```

O `__weak` do template do lua_zephyr continua permitindo override manual.

## 5. Callbacks / eventos

Callback do botão roda no input thread / `sysworkq` (long-press via
`k_work`) → **proibido tocar o `lua_State`**. O lua_zephyr já resolve
comunicação entre threads com **zbus**; reaproveitar:

- `lua_zbook_btn.c` define
  `ZBUS_CHAN_DEFINE(zbook_btn_chan, struct zbook_btn_msg { btn, evt })` +
  message descriptor do lua_zephyr;
- callback C registrado na interface faz
  `zbus_chan_pub(&zbook_btn_chan, &msg, K_NO_WAIT)`;
- Lua consome via observer:

```lua
local btn = zbook.io.btn
btn.enable(btn.BTN0, btn.PRESSED | btn.LONG_PRESSED)

local obs = zephyr.zbus.observer_declare("zbook_lua_btn_obs")
while true do
	local err, chan, msg = obs:wait_msg(1000)
	if err == 0 then
		zephyr.printk("btn " .. msg.btn .. " evt " .. msg.evt)
	end
end
```

Sem fila própria, sem `luaL_ref`. Açúcar futuro: `btn.wait(timeout)`. Mesmo
mecanismo serve depois para IRQ de pino e UART RX.

## 6. API Lua (estilo da versão anterior)

- Enums da interface → constantes na tabela do binding.
- Structs de config → tabelas Lua (como `uart.cfg{...}` antigo).
- Argumento inválido → `luaL_argerror` / `luaL_error`.

```lua
local led = zbook.actuators.led
led.on(led.LED0)                        -- → err
led.blink(led.LED1, led.BLINK_FAST)
led.on_mask(led.LED0 | led.LED2)        -- avaliar aceitar tabela também

local pin = zbook.io.pin
local err, p = pin.set_function(pin.IO_01, { func = "gpio", dir = "out", state = 1 })
p:write(0)
local err, v = p:read()
p:release()
```

Pin: userdata com metatable (`__gc`/`__close` → `zbook_pin_release`). Handles
da interface são estáticos (1 por pino) → **um único userdata por pino**,
cacheado no registry, para o GC de referência antiga não liberar pino em uso.

Mapeamento manual em C (sem SWIG/gerador): API pequena, resultado idiomático.

## 7. Testes e CI

- `tests/` com twister. Alvo preferido `native_sim` — **validar se lua_zephyr
  roda em native_sim**; se não, testes no target com `build_only` no CI.
- Bindings sobre os fake backends da interface; teste executa script Lua e
  confere a chamada C registrada pelo fake.
- CI espelhando a interface: twister, clang-format, build dos samples para
  `zbook@p2/rp2350b/m33`.

## 8. Ordem de execução

Cada passo verificável antes do próximo:

1. Scaffold: `module.yml`, `west.yml` (lua_zephyr), `CMakeLists.txt`,
   `Kconfig` com `ZBOOK_LUA`. Sample `hello` em Lua rodando no HW.
2. Registro genérico por categoria + helper CMake com setup gerado.
3. LED: binding + stub `sdk/` + sample + teste.
4. BTN via zbus.
5. Pin: GPIO → ADC → PWM → UART.
6. Portar UART/SPI antigos quando a interface ganhar `protocols/`.
7. Próximos periféricos conforme a interface crescer (display, sensores, ...).

## 9. Consumindo como aplicação

Aplicação final = repo próprio que é o **manifest (`self`)** e importa
interface + wrapper. Cada import traz suas dependências em cascata.

### `meu-app/west.yml`

```yaml
# Aplicação final usando ZBook interface + wrapper Lua.
#
#   west init -m git@github.com:<org>/meu-app.git zbook-ws
#   cd zbook-ws && west update
#   west build -b zbook@p2/rp2350b/m33 app

manifest:
  version: "0.13"

  remotes:
    - name: zephyr-book
      url-base: https://github.com/zephyr-book

  projects:
    # Interface: import traz zephyr (deps/zephyr + allowlist de módulos em
    # deps/modules) e o board module (zbook/).
    - name: sdk-zbook-interface
      remote: zephyr-book
      revision: v0.1.0            # pinar tag/commit, não main
      path: sdk
      import: true

    # Wrapper Lua: import traz lua_zephyr
    # (deps/modules/lua/lua_zephyr, com submódulo lua/).
    - name: sdk-lua-zbook
      remote: zephyr-book
      revision: v0.1.0
      path: wrapper/lua
      import: true

  self:
    path: app
```

### Workspace resultante

```
zbook-ws/
├── .west/
├── app/                          # repo da aplicação (manifest)
│   ├── west.yml
│   ├── CMakeLists.txt
│   ├── prj.conf
│   └── src/main.lua
├── sdk/                          # sdk-zbook-interface → módulo zbook-interface
├── wrapper/lua/                  # sdk-lua-zbook       → módulo zbook-lua
├── zbook/                        # board module (via interface)
└── deps/
    ├── zephyr/                   # via interface
    └── modules/
        ├── hal/rpi_pico, cmsis_6, ...   # allowlist da interface
        └── lua/lua_zephyr/              # via wrapper
```

### App mínimo

`app/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20.0)
include(${CMAKE_CURRENT_LIST_DIR}/../wrapper/lua/cmake/zbook_lua.cmake)
zbook_lua_define_thread(src/main.lua)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(meu_app)
zbook_lua_generate()
```

`app/prj.conf`:

```
CONFIG_LUA=y
CONFIG_ZBOOK_LUA=y
# CONFIG_ZBOOK_LUA_PIN=n   # desligar o que não usar
```

O `select` do wrapper liga os `ZBOOK_INTERFACE_*` necessários — o app não
precisa conhecer os símbolos da interface.

### Regras do west relevantes

1. **`self` da interface é ignorado quando importada.** O path vem da entrada
   do app (`path: sdk`); mantido igual ao da interface para o layout não mudar.
2. **Precedência:** projetos declarados no manifest raiz vencem os importados;
   entre imports, vence o primeiro. Para fixar outra versão do `lua_zephyr`,
   declarar no app:

   ```yaml
   - name: lua_zephyr
     url: https://github.com/rodrigopex/lua_zephyr
     revision: <outro-commit>
     submodules: true
     path: deps/modules/lua/lua_zephyr
   ```
3. **Interface listando o wrapper:** se o `west.yml` da interface também
   tiver `sdk-lua-zbook`, a entrada do app vence — sem conflito. Recomendado
   manter essa entrada **comentada por padrão** na interface, para quem usa
   só C não baixar Lua.
4. **Allowlist de módulos do Zephyr** vem da interface (`hal_rpi_pico`,
   `cmsis_6`, `picolib`, `littlefs`, `fatfs`). zbus é core do Zephyr, não
   precisa de nada extra. Allowlist importada não pode ser estendida; módulo
   extra (ex.: `nanopb` para message descriptors gerados) entra como projeto
   explícito no app:

   ```yaml
   - name: nanopb
     url: https://github.com/zephyrproject-rtos/nanopb
     revision: <rev usada pelo zephyr v4.4.2>
     path: deps/modules/lib/nanopb
   ```
5. **Pinar versões:** `lua_zephyr` é alpha; interface e wrapper apontam para
   tag/commit, não `main`.

### Variante de desenvolvimento

Topologia atual continua válida: interface como manifest, wrapper como
project nela (ou symlink para o clone local, ver README da interface).
Samples do wrapper:

```sh
west build -b zbook@p2/rp2350b/m33 wrapper/lua/samples/hello
```

## 10. Status da implementação (09/11/2026)

Cobre o que já está na `main` da interface (LED, BTN, pins).

| Passo | Estado | Notas |
|---|---|---|
| 1. Scaffold (`module.yml`, `west.yml`, CMake, Kconfig) | ✅ | `zbook-lua`, `depends: [lua]`; lua_zephyr pinado em `66fc52fd` |
| 2. Registro por categoria + helper CMake | ✅ | `ZBOOK_LUA_BINDING_DEFINE`, `zbook_lua_define_thread()` / `zbook_lua_generate()` |
| 3. LED (`zbook.actuators.led`) | ✅ build | sample `samples/actuators/lua_led`; falta validar no HW |
| 4. BTN via zbus (`zbook.io.btn`) | ✅ build | `btn.wait()` sobre `zbook_lua_btn_sub`; sample `samples/io/lua_btn`; falta HW |
| 5. Pin (`zbook.io.pin`) | 🟡 compila | `default n`: backends `zbook_io_{gpio,adc,pwm,uart}` não estão na main da interface (link falha sem eles) |
| Stubs LuaLS (`sdk/`) | ✅ | `zbook.lua`, `actuators/led.lua`, `io/btn.lua`, `io/pin.lua` |
| Testes (`tests/`) | ⏳ | validar lua_zephyr em `native_sim` |
| CI | ⏳ | |

Builds verificados (`zbook@p2/rp2350b/m33`, Zephyr SDK 1.0.1), sem warnings:

| Sample | FLASH | RAM |
|---|---|---|
| `samples/hello` | 171.9 KB | 50.0 KB |
| `samples/actuators/lua_led` | 175.0 KB | 50.2 KB |
| `samples/io/lua_btn` | 189.3 KB | 53.7 KB |

### Achados durante a implementação

- **Picolibc only.** Zephyr SDK ≥ 1.0 (o que o CI da interface instala) não
  tem Newlib; o `choice` do lua_zephyr (`NEWLIB_LIBC if LUA`) cai em Picolibc,
  e o Lua core (liolib, `luaL_loadfilex`) não linka: falta
  `open/close/read/write/lseek`. Habilitar POSIX completo
  (`POSIX_SYSTEM_INTERFACES` + `POSIX_FILE_SYSTEM` + `POSIX_DEVICE_IO`) ainda
  deixa `lseek` faltando. Solução no wrapper:
  `CONFIG_ZBOOK_LUA_LIBC_STUBS` (default `y if PICOLIBC && !POSIX_DEVICE_IO`),
  stubs fracos com `ENOSYS` em `src/compat/`. Candidato a PR upstream no
  lua_zephyr.
- **REPL do lua_zephyr não tem hook de setup** → `zbook` indisponível no
  REPL. Requer PR upstream (hook fraco tipo `luaz_repl_setup(L)`).
- **`depends: zbook-interface` pendente** no `module.yml` até a interface
  ganhar `name:` no dela.
- **Teste local sem tocar a interface:** enquanto o `west.yml` da interface
  não lista o wrapper, os módulos entram via
  `-DZEPHYR_EXTRA_MODULES="<ws>/deps/modules/lua/lua_zephyr;<ws>/sdk-lua-zbook"`.
  Com a entrada `path: wrapper/lua` + `import: true`, isso some.

## Decisões em aberto

> Adotado provisoriamente na implementação: (1) `err` inteiro,
> (2) `deps/modules/lua/lua_zephyr`, (4) wrapper seleciona pai + filho,
> (5) zbus. (3) segue pendente.

1. **Convenção de retorno:** manter `err` inteiro (`0`/`-errno`, estilo
   antigo — recomendado) ou idiom Lua `nil, msg, errno`?
2. **Path do lua_zephyr:** `deps/modules/lua/lua_zephyr` ou outro?
3. **Interface:** PR adicionando `name: zbook-interface` no `module.yml`?
4. **Kconfig da interface:** wrapper seleciona pai+filho ou interface
   simplifica hierarquia?
5. **Eventos:** zbus (recomendado) ou fila própria com callback Lua direto?
