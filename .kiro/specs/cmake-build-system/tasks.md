# Implementation Plan: CMake 构建系统迁移

## Overview

将现有 Makefile + Visual Studio .vcproj 构建系统迁移到 CMake。按照依赖顺序逐步创建 CMakeLists.txt 文件：先构建底层库（libevent、mongo_c_driver），再配置 LuaJIT IMPORTED 目标，最后构建可执行程序（logic、merge、myxml2）。每个 CMakeLists.txt 文件需处理 Linux/Windows 跨平台差异。

## Tasks

- [x] 1. Create top-level CMakeLists.txt with project definition and global compile options
  - Create `CMakeLists.txt` in project root
  - Set `cmake_minimum_required(VERSION 3.10)` and `project(GameServer C CXX)`
  - Add platform-conditional global compile options: `-g -Wall -march=x86-64` for Linux (GCC), `/W1 /Zi` for Windows (MSVC)
  - Add `add_subdirectory()` calls for libevent, mongo_c_driver, logic, merge, myxml2
  - Configure LuaJIT as an IMPORTED STATIC library target with `INTERFACE_INCLUDE_DIRECTORIES` pointing to `${CMAKE_SOURCE_DIR}/luajit`
  - On Linux: use `find_library(LUAJIT_LIB NAMES luajit-5.1)` with `FATAL_ERROR` if not found
  - On Windows: set `IMPORTED_LOCATION` to `${CMAKE_SOURCE_DIR}/luajit/lua51.lib`
  - _Requirements: 1.1, 1.2, 1.3, 4.1, 4.2, 4.3, 4.4, 8.1, 8.2, 8.3, 8.4_

- [x] 2. Create libevent/CMakeLists.txt for libevent_core static library
  - Define common source files: buffer.c, bufferevent.c, bufferevent_ratelim.c, bufferevent_sock.c, event.c, evmap.c, evutil.c, evutil_rand.c, listener.c, log.c, strlcpy.c, arc4random.c
  - On Linux: add signal.c, exclude win32select.c, buffer_iocp.c, event_iocp.c, bufferevent_async.c
  - On Windows: add win32select.c, buffer_iocp.c, event_iocp.c, bufferevent_async.c, exclude signal.c
  - Create `libevent_core` STATIC library target from the platform-filtered source list
  - Set PUBLIC include directory to `${CMAKE_CURRENT_SOURCE_DIR}/include`
  - On Windows: add PRIVATE include directories for `WIN32-Code` and `compat`
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5_

- [x] 3. Create mongo_c_driver/CMakeLists.txt for mongo_c_driver static library
  - List all source files: bson.c, encoding.c, env.c, md5.c, mongo.c, numbers.c
  - Create `mongo_c_driver` STATIC library target
  - Set PUBLIC include directory to `${CMAKE_CURRENT_SOURCE_DIR}`
  - On Linux: add `target_compile_definitions` for NDEBUG, _POSIX_C_SOURCE=200112L, MONGO_HAVE_STDINT
  - On Windows: add `target_compile_definitions` for INT32_MAX=0x7fffffff, MONGO_STATIC_BUILD, MONGO_USE_LONG_LONG_INT, snprintf=_snprintf
  - _Requirements: 3.1, 3.2, 3.3, 3.4_

- [x] 4. Checkpoint - Verify library targets configure correctly
  - Ensure CMake configuration completes without errors: `mkdir -p build && cd build && cmake ..`
  - Ask the user if questions arise.

- [x] 5. Create logic/CMakeLists.txt for logic executable
  - Use `file(GLOB ...)` to collect `${CMAKE_CURRENT_SOURCE_DIR}/*.cpp` and `${CMAKE_SOURCE_DIR}/common/*.cpp`
  - Create `logic` executable target from collected sources
  - Set `target_include_directories` for: logic, common, luajit, mongo_c_driver, libevent/include (using `${CMAKE_SOURCE_DIR}` paths)
  - Link libraries: libevent_core, mongo_c_driver, luajit
  - On Linux: additionally link pthread, dl, rt; define NDEBUG, _POSIX_C_SOURCE=200112L, MONGO_HAVE_STDINT
  - On Windows: additionally link ws2_32; define WIN32, INT32_MAX=0x7fffffff, MONGO_STATIC_BUILD, MONGO_USE_LONG_LONG_INT, snprintf=_snprintf
  - Add `install(TARGETS logic RUNTIME DESTINATION bin)`
  - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 9.1, 9.2_

- [x] 6. Create merge/CMakeLists.txt for merge executable
  - Use `file(GLOB ...)` to collect `${CMAKE_CURRENT_SOURCE_DIR}/*.cpp` and `${CMAKE_SOURCE_DIR}/common/*.cpp`
  - Create `merge` executable target from collected sources
  - Set `target_include_directories` for: merge, common, logic, luajit, mongo_c_driver, libevent/include
  - Link libraries: libevent_core, mongo_c_driver, luajit
  - On Linux: additionally link pthread, dl, rt
  - On Windows: additionally link ws2_32
  - Add `install(TARGETS merge RUNTIME DESTINATION bin)`
  - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5, 9.1, 9.2_

- [x] 7. Create myxml2/CMakeLists.txt for myxml2 executable
  - Create `myxml2` executable target from myxml2.cpp
  - Set appropriate include directories
  - _Requirements: 7.1, 7.2_

- [x] 8. Update .gitignore to add build directory
  - Append `build/` to the existing `.gitignore` file
  - _Requirements: 10.1, 10.2_

- [x] 9. Final checkpoint - Verify full build and install
  - Ensure all CMake targets configure and build without errors
  - Verify `cmake --install` copies logic and merge to the install prefix bin directory
  - Ensure all tests pass, ask the user if questions arise.

## Notes

- All tasks use CMake as the build configuration language and C/C++ as the compiled language
- No property-based tests are applicable — all acceptance criteria are concrete configuration checks (see design document analysis)
- Checkpoints verify incremental progress by running CMake configure and build
- Each task references specific requirements for traceability
- The existing Makefiles and .vcproj files are preserved (not deleted) for reference
