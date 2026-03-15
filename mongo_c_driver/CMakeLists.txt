# mongo_c_driver static library (Requirements 3.1, 3.2, 3.3, 3.4)

# All source files (Requirement 3.1)
set(MONGO_C_DRIVER_SOURCES
    bson.c
    encoding.c
    env.c
    md5.c
    mongo.c
    numbers.c
)

# Create mongo_c_driver STATIC library (Requirement 3.1)
add_library(mongo_c_driver STATIC
    ${MONGO_C_DRIVER_SOURCES}
)

# Public include directory (Requirement 3.2)
target_include_directories(mongo_c_driver PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific compile definitions
if(UNIX)
    # Linux definitions (Requirement 3.3)
    target_compile_definitions(mongo_c_driver PRIVATE
        NDEBUG
        _POSIX_C_SOURCE=200112L
        MONGO_HAVE_STDINT
    )
elseif(WIN32)
    # Windows definitions (Requirement 3.4)
    target_compile_definitions(mongo_c_driver PRIVATE
        INT32_MAX=0x7fffffff
        MONGO_STATIC_BUILD
        MONGO_USE_LONG_LONG_INT
    )
endif()
