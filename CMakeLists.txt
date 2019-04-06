cmake_minimum_required(VERSION 3.13)
project(compiler C)

set(CMAKE_C_STANDARD 99)

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -lm")

add_executable(compiler src/lexer.c src/compilation_engine.c src/main.c src/util.c src/util.h src/lexer.h src/compilation_engine.h src/symbol_table.c src/symbol_table.h src/vm_writer.c src/vm_writer.h src/common.c src/common.h src/parser.c src/parser.h)

target_link_libraries(compiler "-lm")