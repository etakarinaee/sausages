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
        ${CMAKE_SOURCE_DIR}/src/core/shaders/texture.vert
        ${CMAKE_SOURCE_DIR}/src/core/shaders/texture.frag
        ${CMAKE_SOURCE_DIR}/src/core/shaders/text.vert
        ${CMAKE_SOURCE_DIR}/src/core/shaders/text.frag

        ${CMAKE_SOURCE_DIR}/src/game/client.lua
        ${CMAKE_SOURCE_DIR}/src/game/server.lua
        ${CMAKE_SOURCE_DIR}/src/game/physics.lua

        ${CMAKE_SOURCE_DIR}/data/locales/en.txt
        ${CMAKE_SOURCE_DIR}/data/locales/ru.txt
)

add_custom_command(
        OUTPUT ${ARCHIVE_FILE}
        COMMAND arc c ${ARCHIVE_FILE} ${ARC_INPUTS}
        DEPENDS arc ${ARC_INPUTS}
        VERBATIM
)

add_custom_target(pack_assets ALL DEPENDS ${ARCHIVE_FILE})

# shared sources
set(CORE_SOURCES
        src/core/core.c
        src/core/archive.c
        src/core/lua.c
        src/core/lua_api.c
        src/core/renderer.c
        src/core/net.c
        src/core/cmath.c
        src/core/local.c
        src/core/ui.c
        src/core/audio.c
)

# Freetype2
find_package(Freetype REQUIRED)


set(CORE_FLAGS -g -pedantic-errors -Wall -Wextra -fsanitize=address)
set(CORE_LIBS glad stbi GL glfw freetype luajit-5.1 portaudio m dl)

# client
add_executable(client ${CORE_SOURCES})
target_compile_options(client PRIVATE ${CORE_FLAGS})
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/lib /usr/include/freetype2/)
target_link_libraries(client ${CORE_LIBS})
target_link_options(client PRIVATE -fsanitize=address) # Beccause link with asan
add_dependencies(client pack_assets)

# server
add_executable(server ${CORE_SOURCES})
target_compile_definitions(server PRIVATE SERVER)
target_compile_options(server PRIVATE ${CORE_FLAGS})
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/lib /usr/include/freetype2/)
target_link_libraries(server ${CORE_LIBS})
target_link_options(server PRIVATE -fsanitize=address) # Because link with asan
add_dependencies(server pack_assets)

