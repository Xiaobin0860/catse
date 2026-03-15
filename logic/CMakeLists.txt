# logic executable (Requirements 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 5.7, 9.1, 9.2)

# Collect source files (Requirement 5.1)
file(GLOB LOGIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB COMMON_SOURCES ${CMAKE_SOURCE_DIR}/common/*.cpp)

# Create logic executable
add_executable(logic ${LOGIC_SOURCES} ${COMMON_SOURCES})

# Include directories (Requirement 5.2)
target_include_directories(logic PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/luajit
    ${CMAKE_SOURCE_DIR}/mongo_c_driver
)

# Link libraries (Requirement 5.3)
target_link_libraries(logic PRIVATE libevent_core mongo_c_driver luajit)

# Platform-specific configuration
if(UNIX)
    # Linux link libraries (Requirement 5.4)
    target_link_libraries(logic PRIVATE pthread dl rt)
    # Linux definitions (Requirement 5.6)
    target_compile_definitions(logic PRIVATE
        NDEBUG
        _POSIX_C_SOURCE=200112L
        MONGO_HAVE_STDINT
    )
elseif(WIN32)
    # Windows link libraries (Requirement 5.5)
    target_link_libraries(logic PRIVATE ws2_32)
    # Windows definitions (Requirement 5.7)
    target_compile_definitions(logic PRIVATE
        WIN32
        INT32_MAX=0x7fffffff
        MONGO_STATIC_BUILD
        MONGO_USE_LONG_LONG_INT
    )
endif()

# Install target (Requirement 9.1, 9.2)
install(TARGETS logic RUNTIME DESTINATION bin)
