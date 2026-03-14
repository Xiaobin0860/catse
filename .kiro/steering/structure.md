# Project Structure

```
├── Makefile              # Top-level build: delegates to logic/ and merge/
├── common/               # Shared headers used by both logic and merge
│   ├── common.h          # Platform abstraction (locks, sleep, includes)
│   ├── msg_struct.h      # Network message types (msg_recv, msg_send, msg_inner, msg_admin, msg_http)
│   ├── msg_pool.h        # Lock-based stack pool allocator (msg_stack_pool<T>)
│   ├── msg_queue.h       # Ring-buffer message queues (msg_queue<T>, message_queue<T>)
│   ├── thread_io.h       # Client I/O thread — WebSocket connections, packet parsing
│   ├── thread_admin.h    # Admin console connection thread
│   ├── thread_http.h     # Outbound HTTP request thread
│   ├── thread_inner.h    # Inter-server communication thread (logic↔world)
│   ├── thread_log.h      # Async file logging thread
│   ├── thread_hot.h      # Hot-reload watcher thread
│   ├── thread_loop.h     # Watchdog/deadlock detection thread
│   ├── lua_config_reader.h  # Lua-based config file reader
│   ├── lua_mongo.h/cpp   # MongoDB Lua bindings
│   ├── login_check.h     # Login verification
│   └── msg_inner_parse.h # Inner message parsing
├── logic/                # Main server binary
│   ├── Makefile
│   ├── engine.cpp        # Entry point — initializes and spawns all threads
│   ├── global.h          # Global state, pool init, Lua registration, platform utils
│   ├── thread_logic.h/cpp # Main game logic thread — processes IO/admin/inner queues via Lua
│   ├── msg_ex.h/cpp      # Message serialization — unicast/multicast/broadcast
│   ├── proto_parser.h    # Template-based binary protocol parser (Lua-defined schemas)
│   ├── websocket.h       # WebSocket handshake and framing
│   ├── websocket_request.h/cpp # WebSocket frame parser
│   ├── base64.h/cpp      # Base64 encoding
│   ├── sha1.h/cpp        # SHA-1 for WebSocket handshake
│   ├── lz77.h            # LZ77 compression (unused currently)
│   └── pk_limit.h        # Packet rate limiting
├── merge/                # Database merge utility
│   ├── Makefile
│   └── merge.cpp         # Standalone tool with multiple MongoDB connections for server merges
├── libevent/             # Bundled libevent source (do not modify)
├── luajit/               # LuaJIT headers and prebuilt libs (do not modify)
├── mongo_c_driver/       # Bundled MongoDB C driver source (do not modify)
└── myxml2/               # XML utility (standalone, not part of main build)
```

## Architecture Pattern
- Singleton structs with `static T ref` pattern for all major components
- Each thread struct has `init()`, `run()`, and static callback trampolines (`static_cb_*`)
- Inter-thread communication via lock-free ring-buffer queues (`msg_queue<T>`)
- Memory managed through pre-allocated pool allocators (`msg_stack_pool<T>`)
- Lua functions registered as global tables (e.g., `lua_mongo`, `lua_log`, `global`, `thread_http`)

## Key Conventions
- Headers in `common/` are shared; `logic/` headers are server-specific
- Thread structs are defined entirely in headers (inline implementation)
- Static callback functions bridge C-style libevent callbacks to struct methods
- All network data uses network byte order (htons/htonl/ntohl)
- Lua stack balance is verified with `top_old`/`top_new` assertions
- `assert(0)` is used as the error handling pattern throughout
