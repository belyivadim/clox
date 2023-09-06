cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME clox)

project(${PROJECT_NAME})

file(GLOB_RECURSE HEADERS "./src/*.h")
file(GLOB_RECURSE SOURCES "./src/*.c")

include_directories(src)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_compile_options(${PROJECT_NAME} PRIVATE -std=c99 -pedantic-errors -Wall -Wextra -g)
