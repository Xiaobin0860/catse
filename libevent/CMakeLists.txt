# libevent_core static library (Requirements 2.1, 2.2, 2.3, 2.4, 2.5)
#
# NOTE: The bundled libevent source is incomplete on Linux (missing epoll.c,
# poll.c, select.c backends). On Linux we create an IMPORTED target wrapping
# the system-installed libevent. On Windows we build from bundled source.

if(UNIX)
    # Linux: use system libevent, expose bundled headers
    find_library(LIBEVENT_LIB NAMES event)
    if(NOT LIBEVENT_LIB)
        message(FATAL_ERROR "Could not find libevent. Install with: sudo apt install libevent-dev")
    endif()
    add_library(libevent_core STATIC IMPORTED GLOBAL)
    set_target_properties(libevent_core PROPERTIES
        IMPORTED_LOCATION "${LIBEVENT_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif(WIN32)
    # Windows: build from bundled source
    set(LIBEVENT_COMMON_SOURCES
        buffer.c
        bufferevent.c
        bufferevent_ratelim.c
        bufferevent_sock.c
        event.c
        evmap.c
        evutil.c
        evutil_rand.c
        listener.c
        log.c
        strlcpy.c
    )
    set(LIBEVENT_PLATFORM_SOURCES
        win32select.c
        buffer_iocp.c
        event_iocp.c
        bufferevent_async.c
    )
    add_library(libevent_core STATIC
        ${LIBEVENT_COMMON_SOURCES}
        ${LIBEVENT_PLATFORM_SOURCES}
    )
    target_include_directories(libevent_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_include_directories(libevent_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/WIN32-Code
        ${CMAKE_CURRENT_SOURCE_DIR}/compat
    )
endif()
