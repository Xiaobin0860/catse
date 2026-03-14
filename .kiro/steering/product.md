# Product Overview

A C++ game server engine with an embedded LuaJIT scripting layer. The server handles real-time client connections over WebSocket, supports inter-server communication (cross-server/跨服 mode), and persists data via MongoDB.

Core capabilities:
- Multi-threaded event-driven networking (libevent) with WebSocket transport
- Lua-scripted game logic with hot-reload support (modify `script/Main.lua` at runtime)
- Binary protocol serialization with template-based message parsing
- MongoDB persistence with Lua bindings
- Admin console and HTTP callback interfaces
- A standalone `merge` tool for cross-database MongoDB operations (server merges)

Configuration is driven by `script/Config.lua`. Game logic lives in Lua scripts under `script/`.
