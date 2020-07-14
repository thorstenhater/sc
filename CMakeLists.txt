cmake_minimum_required(VERSION 3.17)

project(SimpleCompiler VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")

set(CMAKE_CXX_FLAGS "-I/usr/local/opt/llvm/include/c++/v1/")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(sc main.cpp)
target_sources(sc PUBLIC AST.hpp AST.cpp)
target_sources(sc PUBLIC Types.hpp)
target_sources(sc PUBLIC CPS.hpp CPS.cpp)
target_sources(sc PUBLIC TailCPS.hpp TailCPS.cpp)
