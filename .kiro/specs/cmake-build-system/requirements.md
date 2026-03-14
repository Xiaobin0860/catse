# 需求文档

## 简介

将现有游戏服务器框架的构建系统从 Makefile + Visual Studio .vcproj 迁移到 CMake。项目包含多个组件：logic（游戏逻辑服务器）、merge（合服工具）、myxml2（XML工具），以及捆绑的第三方库 libevent、luajit、mongo_c_driver。CMake 构建系统需要同时支持 Linux 和 Windows 平台，保持与现有构建行为一致。

## 术语表

- **CMake_构建系统**: 基于 CMakeLists.txt 文件的跨平台构建配置系统
- **logic**: 游戏逻辑服务器可执行程序，包含 websocket、protobuf 解析、Lua 脚本引擎等功能
- **merge**: 合服工具可执行程序
- **myxml2**: XML 处理工具可执行程序
- **libevent**: 捆绑的网络事件库，以源码形式编译为静态库
- **luajit**: 捆绑的 LuaJIT 库，Linux 下链接系统安装版本，Windows 下链接预编译的 lua51.lib
- **mongo_c_driver**: 捆绑的 MongoDB C 驱动，以源码形式编译为静态库
- **common**: 共享头文件和源文件目录，被 logic 和 merge 共同引用
- **install_目标**: 将编译产物复制到 `../../bin` 目录的部署操作

## 需求

### 需求 1：顶层 CMake 项目结构

**用户故事：** 作为开发者，我希望有一个顶层 CMakeLists.txt 来统一管理所有子项目，以便用一条命令构建整个项目。

#### 验收标准

1. THE CMake_构建系统 SHALL 提供一个顶层 CMakeLists.txt 文件，设置最低 CMake 版本要求（3.10 或以上）并定义项目名称
2. THE CMake_构建系统 SHALL 通过 add_subdirectory 将 logic、merge、myxml2 作为子项目纳入构建
3. THE CMake_构建系统 SHALL 将 libevent、mongo_c_driver 构建为内部静态库供子项目链接
4. WHEN 开发者在项目根目录执行 `cmake --build .` 时，THE CMake_构建系统 SHALL 构建所有可执行目标（logic、merge、myxml2）

### 需求 2：libevent 静态库构建

**用户故事：** 作为开发者，我希望捆绑的 libevent 源码能被编译为静态库，以便 logic 和 merge 可以链接使用。

#### 验收标准

1. THE CMake_构建系统 SHALL 将 libevent 目录下的 C 源文件编译为名为 `libevent_core` 的静态库
2. THE CMake_构建系统 SHALL 将 `libevent/include` 设置为 libevent_core 的公共头文件搜索路径
3. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 额外将 `libevent/WIN32-Code` 和 `libevent/compat` 添加为头文件搜索路径
4. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 排除 `signal.c` 并包含 `win32select.c`、`buffer_iocp.c`、`event_iocp.c`、`bufferevent_async.c` 等 Windows 专用源文件
5. WHILE 在 Linux 平台构建时，THE CMake_构建系统 SHALL 排除 `win32select.c`、`buffer_iocp.c`、`event_iocp.c`、`bufferevent_async.c` 等 Windows 专用源文件

### 需求 3：mongo_c_driver 静态库构建

**用户故事：** 作为开发者，我希望捆绑的 MongoDB C 驱动源码能被编译为静态库，以便 logic 和 merge 可以链接使用。

#### 验收标准

1. THE CMake_构建系统 SHALL 将 mongo_c_driver 目录下的所有 C 源文件（bson.c、encoding.c、env.c、md5.c、mongo.c、numbers.c）编译为名为 `mongo_c_driver` 的静态库
2. THE CMake_构建系统 SHALL 将 `mongo_c_driver` 目录设置为该静态库的公共头文件搜索路径
3. WHILE 在 Linux 平台构建时，THE CMake_构建系统 SHALL 为 mongo_c_driver 定义预处理宏 `NDEBUG`、`_POSIX_C_SOURCE=200112L`、`MONGO_HAVE_STDINT`
4. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 为 mongo_c_driver 定义预处理宏 `INT32_MAX=0x7fffffff`、`MONGO_STATIC_BUILD`、`MONGO_USE_LONG_LONG_INT`、`snprintf=_snprintf`

### 需求 4：LuaJIT 依赖配置

**用户故事：** 作为开发者，我希望 CMake 能正确配置 LuaJIT 依赖，以便在 Linux 和 Windows 上都能链接 LuaJIT。

#### 验收标准

1. THE CMake_构建系统 SHALL 创建一个 IMPORTED 库目标来表示 LuaJIT 依赖
2. THE CMake_构建系统 SHALL 将 `luajit` 目录设置为 LuaJIT 的头文件搜索路径
3. WHILE 在 Linux 平台构建时，THE CMake_构建系统 SHALL 链接系统安装的 `luajit-5.1` 共享库
4. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 链接项目内的 `luajit/lua51.lib` 静态库

### 需求 5：logic 可执行程序构建

**用户故事：** 作为开发者，我希望 logic 游戏逻辑服务器能通过 CMake 正确编译和链接，以便替代原有的 logic/Makefile。

#### 验收标准

1. THE CMake_构建系统 SHALL 将 logic 目录下的所有 .cpp 文件和 common 目录下的所有 .cpp 文件编译并链接为名为 `logic` 的可执行程序
2. THE CMake_构建系统 SHALL 为 logic 设置头文件搜索路径，包含 logic、common、luajit、mongo_c_driver、libevent/include 目录
3. THE CMake_构建系统 SHALL 将 logic 链接 libevent_core、mongo_c_driver、LuaJIT 静态库
4. WHILE 在 Linux 平台构建时，THE CMake_构建系统 SHALL 为 logic 额外链接 pthread、dl、rt 系统库
5. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 为 logic 额外链接 ws2_32 系统库
6. WHILE 在 Linux 平台构建时，THE CMake_构建系统 SHALL 为 logic 定义预处理宏 `NDEBUG`、`_POSIX_C_SOURCE=200112L`、`MONGO_HAVE_STDINT`
7. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 为 logic 定义预处理宏 `WIN32`、`INT32_MAX=0x7fffffff`、`MONGO_STATIC_BUILD`、`MONGO_USE_LONG_LONG_INT`、`snprintf=_snprintf`

### 需求 6：merge 可执行程序构建

**用户故事：** 作为开发者，我希望 merge 合服工具能通过 CMake 正确编译和链接，以便替代原有的 merge/Makefile。

#### 验收标准

1. THE CMake_构建系统 SHALL 将 merge 目录下的所有 .cpp 文件和 common 目录下的所有 .cpp 文件编译并链接为名为 `merge` 的可执行程序
2. THE CMake_构建系统 SHALL 为 merge 设置头文件搜索路径，包含 merge、common、logic、luajit、mongo_c_driver、libevent/include 目录
3. THE CMake_构建系统 SHALL 将 merge 链接 libevent_core、mongo_c_driver、LuaJIT 静态库
4. WHILE 在 Linux 平台构建时，THE CMake_构建系统 SHALL 为 merge 额外链接 pthread、dl、rt 系统库
5. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 为 merge 额外链接 ws2_32 系统库

### 需求 7：myxml2 可执行程序构建

**用户故事：** 作为开发者，我希望 myxml2 XML 工具能通过 CMake 正确编译和链接。

#### 验收标准

1. THE CMake_构建系统 SHALL 将 myxml2/myxml2.cpp 编译并链接为名为 `myxml2` 的可执行程序
2. THE CMake_构建系统 SHALL 为 myxml2 设置正确的头文件搜索路径

### 需求 8：跨平台编译选项

**用户故事：** 作为开发者，我希望 CMake 构建系统能根据平台自动设置正确的编译选项，以便在 Linux 和 Windows 上都能正确编译。

#### 验收标准

1. WHILE 在 Linux 平台构建时，THE CMake_构建系统 SHALL 设置编译选项 `-g -Wall -march=x86-64`
2. WHILE 在 Windows 平台构建时，THE CMake_构建系统 SHALL 使用 MSVC 编译器的对应警告级别和调试信息选项
3. THE CMake_构建系统 SHALL 支持 Debug 和 Release 两种构建类型
4. WHEN 构建类型为 Release 时，THE CMake_构建系统 SHALL 启用编译器优化

### 需求 9：安装目标

**用户故事：** 作为开发者，我希望能通过 `cmake --install` 将编译产物部署到指定目录，以便替代原有 Makefile 的 `make install` 功能。

#### 验收标准

1. WHEN 执行安装操作时，THE CMake_构建系统 SHALL 将 logic 和 merge 可执行文件复制到可配置的安装目录（默认为 `${CMAKE_INSTALL_PREFIX}/bin`）
2. THE CMake_构建系统 SHALL 支持通过 CMAKE_INSTALL_PREFIX 变量自定义安装路径

### 需求 10：构建目录隔离

**用户故事：** 作为开发者，我希望构建产物与源码目录分离，以便保持源码目录整洁。

#### 验收标准

1. THE CMake_构建系统 SHALL 支持 out-of-source 构建（在独立的 build 目录中生成所有构建产物）
2. THE CMake_构建系统 SHALL 在 .gitignore 中添加 `build/` 目录的忽略规则
