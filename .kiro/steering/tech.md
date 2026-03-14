# Tech Stack & Build System

## Language
- C++ (with C for mongo_c_driver). No C++ standard explicitly set; code uses C++03-compatible patterns.
- Lua (LuaJIT 5.1) for game logic scripting.

## Key Libraries
- **libevent** — event-driven networking (bundled in `libevent/`, headers in `libevent/include/event2/`)
- **LuaJIT 5.1** — embedded scripting engine (headers/libs in `luajit/`)
- **mongo-c-driver** — MongoDB client (bundled in `mongo_c_driver/`)

## Build System
- GNU Make. No CMake or autotools.
- Top-level `Makefile` delegates to `logic/Makefile` and `merge/Makefile`.
- Cross-platform support via `#ifdef __VERSION__` (Linux) vs Windows (`windows.h`, `.vcproj` files).

## Build Commands
```bash
# Build everything (logic + merge)
make

# Clean all build artifacts
make clean

# Install binaries to ../../bin
make install
```

## Compiler Flags
```
-DNDEBUG -D_POSIX_C_SOURCE=200112L -DMONGO_HAVE_STDINT -g -Wall -march=x86-64
```

## Linker Dependencies (Linux)
```
-lluajit-5.1 -levent -lpthread -ldl -lrt -lstdc++
```

## Build Outputs
- `logic/logic` — main game server binary
- `merge/merge` — database merge utility

## Platform Notes
- Linux: uses pthreads, `daemon()`, POSIX APIs, atomic spinlocks (`__sync_lock_test_and_set`)
- Windows: uses `CreateThread`, `CRITICAL_SECTION`, `.vcproj` project files, `core.sln`
- Platform branching is done via `#ifdef __VERSION__` (GCC/Linux) vs `#else` (MSVC/Windows)

## Runtime Configuration
- `script/Config.lua` — server config (ports, pool sizes, cross-server flag)
- `script/Main.lua` — main game logic entry point (hot-reloadable)
- `script/merge.lua` — merge tool script
