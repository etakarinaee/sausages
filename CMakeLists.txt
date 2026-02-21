cmake_minimum_required(VERSION 4.1)
project(sausages C)

set(CMAKE_C_STANDARD 90)

add_executable(arc tools/arc.c core_archive.c)

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/sausages.arc
        COMMAND arc c ${CMAKE_BINARY_DIR}/sausages.arc
        ${CMAKE_SOURCE_DIR}/shaders/tri.frag
        ${CMAKE_SOURCE_DIR}/shaders/tri.vert
        DEPENDS arc shaders/tri.frag shaders/tri.vert
)

add_custom_target(shaders ALL DEPENDS ${CMAKE_BINARY_DIR}/sausages.arc)

add_executable(sausages core.c core_archive.c)
add_dependencies(sausages shaders)

target_link_libraries(sausages GL GLU GLEW glut)