# merge executable (Requirements 6.1, 6.2, 6.3, 6.4, 6.5, 9.1, 9.2)

# Collect source files (Requirement 6.1)
file(GLOB MERGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB COMMON_SOURCES ${CMAKE_SOURCE_DIR}/common/*.cpp)

# Create merge executable
add_executable(merge ${MERGE_SOURCES} ${COMMON_SOURCES})

# Include directories (Requirement 6.2)
target_include_directories(merge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/logic
    ${CMAKE_SOURCE_DIR}/luajit
    ${CMAKE_SOURCE_DIR}/mongo_c_driver
    ${CMAKE_SOURCE_DIR}/libevent/include
)

# Link libraries (Requirement 6.3)
target_link_libraries(merge PRIVATE libevent_core mongo_c_driver luajit)

# Platform-specific configuration
if(UNIX)
    # Linux link libraries (Requirement 6.4)
    target_link_libraries(merge PRIVATE pthread dl rt)
    # Linux definitions (same as logic)
    target_compile_definitions(merge PRIVATE
        NDEBUG
        _POSIX_C_SOURCE=200112L
        MONGO_HAVE_STDINT
    )
elseif(WIN32)
    # Windows link libraries (Requirement 6.5)
    target_link_libraries(merge PRIVATE ws2_32)
    target_compile_definitions(merge PRIVATE
        WIN32
        INT32_MAX=0x7fffffff
        MONGO_STATIC_BUILD
        MONGO_USE_LONG_LONG_INT
        snprintf=_snprintf
    )
endif()

# Install target (Requirement 9.1, 9.2)
install(TARGETS merge RUNTIME DESTINATION bin)
