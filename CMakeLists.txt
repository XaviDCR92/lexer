set(project lexer)
cmake_minimum_required(VERSION 3.5)
project(${project})
add_executable(${project})
target_compile_options(${project} PRIVATE -Wall -Wextra --pedantic -ansi)
target_sources(${project} PRIVATE "lexer.c" "token.c" "parser.c")

