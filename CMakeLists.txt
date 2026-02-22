cmake_minimum_required(VERSION 4.1)
project(sausages C)

set(CMAKE_C_STANDARD 90)
add_compile_options(-pedantic-errors -Wall -Wextra)

add_executable(arc tools/arc.c core_archive.c)

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/sausages.arc
        COMMAND arc c ${CMAKE_BINARY_DIR}/sausages.arc
        ${CMAKE_SOURCE_DIR}/shaders/tri.frag
        ${CMAKE_SOURCE_DIR}/shaders/tri.vert
        ${CMAKE_SOURCE_DIR}/game.lua
        DEPENDS arc shaders/tri.frag shaders/tri.vert game.lua
)

add_custom_target(shaders ALL DEPENDS ${CMAKE_BINARY_DIR}/sausages.arc)

add_executable(sausages core.c core_archive.c core_lua.c core_lua_api.c core_renderer.c glad.c)
add_dependencies(sausages shaders)

target_include_directories(sausages PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(sausages GL glfw luajit-5.1 m dl)
