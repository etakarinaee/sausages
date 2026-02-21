cmake_minimum_required(VERSION 4.1)
project(sausages C)

set(CMAKE_C_STANDARD 90)

add_executable(sausages main.c
        archive.c
        archive.h)

target_link_libraries(sausages GL GLU GLEW glut)
