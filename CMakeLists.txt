cmake_minimum_required(VERSION 4.1)

project(sausages C)

set(CMAKE_C_STANDARD 99)

# glad must be its own target so -pedantic-errors does not apply on it
add_library(glad STATIC lib/glad/glad.c)
target_include_directories(glad PRIVATE ${CMAKE_SOURCE_DIR}/lib)

# stbi
add_library(stbi STATIC lib/stbi/stb_image.c)
target_include_directories(stbi PRIVATE ${CMAKE_SOURCE_DIR}/lib)

add_executable(arc tools/arc.c src/core/archive.c)
target_compile_options(arc PRIVATE -pedantic-errors -Wall -Wextra)

# archive assets
set(ARCHIVE_FILE ${CMAKE_BINARY_DIR}/sausages.arc)

set(ARC_INPUTS
        ${CMAKE_SOURCE_DIR}/src/core/shaders/quad.vert
        ${CMAKE_SOURCE_DIR}/src/core/shaders/quad.frag
        ${CMAKE_SOURCE_DIR}/src/game/game.lua
)

add_custom_command(
        OUTPUT ${ARCHIVE_FILE}
        COMMAND arc c ${ARCHIVE_FILE} ${ARC_INPUTS}
        DEPENDS arc ${ARC_INPUTS}
        VERBATIM
)

add_custom_target(pack_assets ALL DEPENDS ${ARCHIVE_FILE})

add_executable(sausages src/core/core.c src/core/archive.c src/core/lua.c src/core/lua_api.c src/core/renderer.c)

target_compile_options(sausages PRIVATE -pedantic-errors -Wall -Wextra)
target_include_directories(sausages PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(sausages glad stbi GL glfw luajit-5.1 m dl)

add_dependencies(sausages pack_assets)
