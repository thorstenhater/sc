cmake_minimum_required(VERSION 3.18)

include(ExternalProject)

project(SimpleCompiler VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")

set(CMAKE_CXX_FLAGS "-I/usr/local/opt/llvm/include/c++/v1/")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(cps STATIC AST.hpp AST.cpp Types.hpp Types.cpp TailCPS.hpp TailCPS.cpp Simplify.hpp)

add_executable(sc main.cpp)
target_link_libraries(sc PUBLIC cps)

ExternalProject_Add(chibi
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/chibi-scheme
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "make"
  BUILD_IN_SOURCE ON
  INSTALL_COMMAND make install)

file(COPY ${CMAKE_SOURCE_DIR}/compiler DESTINATION ${CMAKE_BINARY_DIR})

add_custom_target(stub
  COMMAND chibi-ffi ${CMAKE_SOURCE_DIR}/interface.stub
  COMMAND mv ${CMAKE_SOURCE_DIR}/interface.c ${CMAKE_BINARY_DIR}/interface.cxx
  BYPRODUCTS ${CMAKE_BINARY_DIR}/interface.cxx)

add_library(interface SHARED ${CMAKE_SOURCE_DIR}/interface.h ${CMAKE_BINARY_DIR}/interface.cxx)
add_dependencies(interface stub)
target_include_directories(interface PUBLIC "/usr/local/include/")
target_include_directories(interface PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(interface PUBLIC -L/usr/local/lib/ -lchibi-scheme)
target_link_libraries(interface PUBLIC cps)

add_custom_command(TARGET interface
  POST_BUILD VERBATIM
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:interface>" ${CMAKE_BINARY_DIR}/compiler)
