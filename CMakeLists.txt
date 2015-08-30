cmake_minimum_required(VERSION 2.8)

find_package(LLVM REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
add_definitions( -D__STDC_LIMIT_MACROS )
add_definitions( -D__STDC_CONSTANT_MACROS )
include_directories(${LLVM_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "-std=c++11 -fno-rtti -fno-exceptions ${CMAKE_CXX_FLAGS}")

add_library(dcc888 MODULE AliasSet.cpp)
