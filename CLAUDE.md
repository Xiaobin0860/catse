# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working
with code in this repository.

## Build Commands

### CMake (recommended, cross-platform)

```bash
cd build
cmake ..
make -j$(nproc)
```

Outputs go to `bin/` (logic, merge, myxml2) and `lib/` (static libs).

### Legacy Make (Linux only)

```bash
make        # builds logic and merge via their subdirectory Makefiles
make clean
make install # copies binaries to bin/
```

The Make build compiles `mongo_c_driver/*.c` directly into the logic binary
(no separate libmongoc).

### Dependencies

- **LuaJIT 5.1**: `sudo apt install libluajit-5.1-dev` (system shared library)
- **libevent 2.x**: vendored in `libevent/`, built as a static lib via CMake
- **mongo-c-driver**: vendored in `mongo_c_driver/`, built as a static lib via CMake
- **pthread, dl, rt**: system libraries

## Architecture

This is the **C++ server engine** for the MMORPG. It is a thin C++ host that
runs LuaJIT — all game logic lives in `cats/server/script/`. The engine
provides networking, MongoDB access, binary protocol parsing, logging, and
hot-reload; Lua calls back into C++ for all I/O.

### Thread Model (8 threads + main)

The engine is a multi-threaded event-driven server built on **libevent**.
Each thread runs its own `event_base` dispatch loop and communicates via
lock-based ring-buffer message queues.

| Thread | Role | Port |
| ------ | ---- | ---- |
| `thread_io` | Client I/O — WebSocket for game clients | `PORT_CLIENT` (8100) |
| `thread_logic` | Game logic — LuaJIT, dispatches to Lua handlers | — |
| `thread_inner` | Cross-server: normal ↔ middle/world | `MIDDLE_PORT` (20000) |
| `thread_admin` | Admin TCP for GM tools | `PORT_ADMIN` (10000) |
| `thread_http` | Outbound HTTP client (pay callbacks) | via config |
| `thread_log` | Disk logging for all threads | — |
| `thread_hot` | Hot-reload watcher (polls `Main.lua` mtime) | — |
| `thread_loop` | Watchdog: stuck logic thread, Lua stack dump | — |

**Startup sequence** (`logic/engine.cpp:23-79`): init all threads → create
OS threads → drop Lua config reader → enter infinite sleep on main thread.

### thread_logic Main Loop

The core game loop runs each tick in `thread_logic::run()`:

1. **handle_io()** — Pops `msg_recv` from I/O queue, calls Lua
   `handlerMsg(fd, id)`. Also processes disconnect messages.
2. **handle_inner()** — Pops `msg_inner` from cross-server queue, calls Lua
   `handlerInner(fd)`.
3. **handle_admin()** — Pops `msg_admin` from admin queue, calls Lua
   `handlerAdmin(fd, data)`, sends response back.
4. **handle_time()** — Calls Lua `handlerTime(msec)` for timer-based game
   logic.
5. **hot_update()** — If `thread_hot` detected a `Main.lua` mtime change,
   re-executes `Main.lua` via `luaL_loadfile` + `lua_pcall` without
   restarting the process.

Each handler checks for Lua stack balance before/after to detect leaks.

### Message Flow & Binary Protocol

```text
Client --WebSocket--> thread_io --msg_recv--> thread_logic --> Lua handlerMsg
                                                                    │
Lua unicast/broadcast/multicast <-- msg_send queue -- thread_io <──┘
```

- **Protocol format**: `msg_head` (4 bytes: `short len` + `short id`) +
  payload. Wrapped in WebSocket binary frames.
- **Parsing**: `proto_parser` (`logic/proto_parser.h`) uses a DFS-based
  tree structure with pre-registered templates. Max 32768 nodes × 128
  fields per node. Templates are registered from Lua via
  `msg_ex.register_template(id, template_table)`.
- **Message types**: `msg_recv` (512B buf), `msg_send` (16KB buf, up to
  2048 target FDs), `msg_inner` (200KB buf), `msg_admin` (512KB buf),
  `msg_http` (512KB buf)
- **Memory pools**: `msg_stack_pool<T>` — pre-allocated, lock-protected
  stack allocator. All messages are pooled; allocation failure triggers
  assert.
- **Queues**: `msg_queue<T>` — ring buffer for multi-producer scenarios.
  `message_queue<T>` — ring buffer for single-producer/single-consumer
  (owns storage).

### Lua Bindings

The engine exposes these C++ modules to Lua (all registered in
`thread_logic::init()`):

| Lua Table | C++ Struct | Purpose |
| --------- | ---------- | ------- |
| `global` | `global` | `date`, `get_msec`, `get_ip_from_fd`, file stat |
| `lua_mongo` | `lua_mongo` | MongoDB CRUD: auth, find, insert, update, etc. |
| `lua_log` | `thread_log` | `reg(filename, ...)`, `write(logid, ...)` |
| `thread_http` | `thread_http` | `send(url, data)` — async HTTP GET |
| `msg_ex` | `msg_ex` | Proto read/write, unicast, broadcast, multicast |
| `msg_inner_parse` | `msg_inner_parse` | Cross-server msg read/write |
| `pk_limit` | `pk_limit` | Per-protocol rate limiting |

### Configuration

Config is read at startup from a Lua file (default: `script/Config.lua`,
override with `--config=` or `ENGINE_CONFIG_LUA` env var).
`lua_config_reader` evaluates `return KEY` expressions in a sandbox Lua
state. Key config values: `PORT_CLIENT`, `PORT_ADMIN`, `PORT_HTTP`,
`IP_HTTP`, `MIDDLE_PORT`, `IS_MIDDLE`, `SVR_INDEX`, `DISCONNECT_PROTO_ID`,
and various `Q_CAP_*` queue capacities.

### Cross-Server Architecture

`thread_inner` operates in two modes based on `IS_MIDDLE` config:

- **Normal server** (`IS_MIDDLE=false`): Connects as a client to the
  middle/world server. Sends `PACKET_ID_HELLO` with `SVR_INDEX` on connect.
  Handles disconnect/reconnect.
- **Middle server** (`IS_MIDDLE=true`): Listens on `MIDDLE_PORT`, accepts
  connections from multiple normal servers. Maintains FD tracking for all
  connected servers.

The middle server can be dynamically connected via Lua
`thread_inner.set_connect(ip, port)`.

### Hot-Reload

`thread_hot` polls `Main.lua` mtime every ~100ms. When changed,
`thread_logic::hot_update()` calls `luaL_loadfile` + `lua_pcall` on
`Main.lua`, re-registering all Lua handlers in-process. Lua state persists
across reloads. All non-logic threads continue running unaffected.

### Merge Tool (`merge/`)

Standalone binary for cross-server data merging. Opens 7 independent
MongoDB connections (`lua_mongo1`–`lua_mongo7`), runs `script/merge.lua`.
Shares the same BSON ↔ Lua conversion logic as the main engine but
duplicated in `merge.cpp` with small differences (e.g., uses `bson_oid_gen`
instead of custom OID generation).

### myxml2 (`myxml2/`)

XML-to-Lua config converter used by the `catd/` data pipeline. Separate
tool, not part of the running server.

## Key Source Files

| File | Role |
| ---- | ---- |
| `logic/engine.cpp` | `main()` — config parsing, thread init, startup |
| `logic/thread_logic.cpp` | Core game loop: io, inner, admin, time, hot |
| `logic/proto_parser.h` | Binary protocol parser/serializer (DFS, templates) |
| `logic/websocket.h` | WebSocket handshake (SHA1+Base64), frame encode/decode |
| `logic/msg_ex.h` | Lua-facing msg read/write, broadcast, multicast |
| `common/msg_struct.h` | Message types with binary read/write primitives |
| `common/msg_pool.h` | Lock-protected pre-allocated message pools |
| `common/msg_queue.h` | Ring buffer queues (two variants) |
| `common/lua_mongo.h` | MongoDB bridge — BSON ↔ Lua table, OID generation |
| `common/lua_config_reader.h` | Lua-based config reader |
| `common/thread_io.h` | Client I/O thread — WebSocket + libevent bufferevent |
| `common/thread_inner.h` | Cross-server thread — normal/middle dual mode |
| `common/thread_admin.h` | Admin TCP thread |
| `common/thread_http.h` | Outbound HTTP client thread |
| `common/thread_log.h` | Logging thread with file rotation, Lua integration |
| `common/thread_hot.h` | Hot-reload file watcher thread |
| `common/thread_loop.h` | Watchdog thread with Lua stack dump |
| `merge/merge.cpp` | Standalone data merge binary |
| `myxml2/myxml2.cpp` | XML-to-Lua config converter |

## Platform Support

Code compiles on Linux (GCC) and Windows (MSVC). Platform divergence is
handled with `#ifdef __VERSION__` (Linux) / `#else` (Windows). Linux:
pthreads, `syscall(SYS_gettid)`, signals, `nanosleep`. Windows:
`CreateThread`, `CRITICAL_SECTION`, `Sleep`. The Windows port is legacy and
may not be fully maintained.

## Wire Compatibility

The Rust engine (`rcate/`) maintains binary wire-protocol compatibility
with this C++ engine. Changes to protocol parsing in `proto_parser.h`,
message struct layouts in `msg_struct.h`, or WebSocket framing in
`websocket.h` must preserve byte-level compatibility. The
`msg_utils::cvt()` double endianness handling and LZ77 compression behavior
must match exactly.
