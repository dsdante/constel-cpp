cmake_minimum_required(VERSION 3.14)
project(constel-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fms-extensions")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin-$<LOWER_CASE:$<CONFIG>>)

include_directories(/usr/include/freetype2)
add_executable(constel
        constel.cpp
        common.cpp
        graphics.cpp
        input.cpp
        world.cpp)

target_link_libraries(constel m pthread GL GLEW glfw freetype)

# Copy config and shaders
add_custom_command(TARGET constel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different *.frag ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different *.vert ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different constel.conf ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )