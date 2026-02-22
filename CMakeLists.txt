project(sausages C)

set(CMAKE_C_STANDARD 90)

# glad must be its own target so -pedantic-errors does not apply on it
add_library(glad STATIC glad.c)

add_executable(arc tools/arc.c core_archive.c)
target_compile_options(arc PRIVATE -pedantic-errors -Wall -Wextra)

add_executable(sausages core.c core_archive.c core_lua.c core_lua_api.c core_renderer.c)
target_compile_options(sausages PRIVATE -pedantic-errors -Wall -Wextra)
target_link_libraries(sausages glad GL glfw luajit-5.1 m dl)
