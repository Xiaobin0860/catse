cmake_minimum_required(VERSION 3.10)
project(GameServer C CXX)

# ---------------------------------------------------------------------------
# Global output directories
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Ensure multi-config generators (Visual Studio, Xcode) also use bin/lib.
foreach(cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG_UPPER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG_UPPER} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
endforeach()

# ---------------------------------------------------------------------------
# Global compile options (Requirement 8.1, 8.2, 8.3, 8.4)
# ---------------------------------------------------------------------------
if(UNIX)
    add_compile_options(-g -Wall -march=x86-64)
elseif(MSVC)
    add_compile_options(/W1 /Zi)
endif()

# ---------------------------------------------------------------------------
# LuaJIT IMPORTED target (Requirement 4.1, 4.2, 4.3, 4.4)
# ---------------------------------------------------------------------------
add_library(luajit STATIC IMPORTED GLOBAL)
set_target_properties(luajit PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/luajit"
)

if(UNIX)
    find_library(LUAJIT_LIB NAMES luajit-5.1)
    if(NOT LUAJIT_LIB)
        message(FATAL_ERROR "Could not find luajit-5.1 library. Install with: sudo apt install libluajit-5.1-dev")
    endif()
    set_target_properties(luajit PROPERTIES
        IMPORTED_LOCATION "${LUAJIT_LIB}"
    )
elseif(WIN32)
    set_target_properties(luajit PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/luajit/lua51.lib"
        INTERFACE_LINK_LIBRARIES "legacy_stdio_definitions;legacy_stdio_wide_specifiers"
    )
endif()

# ---------------------------------------------------------------------------
# Subdirectories (Requirement 1.2, 1.3)
# ---------------------------------------------------------------------------
add_subdirectory(libevent)
add_subdirectory(mongo_c_driver)
add_subdirectory(logic)
add_subdirectory(merge)
add_subdirectory(myxml2)
