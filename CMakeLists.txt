cmake_minimum_required(VERSION 3.10)
project(tedit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tedit src/main.cpp)

target_include_directories(tedit PRIVATE include)
target_sources(tedit PRIVATE src/editor.cpp src/editor.h)
