# myxml2 executable (Requirements 7.1, 7.2)

add_executable(myxml2 myxml2.cpp)

target_include_directories(myxml2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
