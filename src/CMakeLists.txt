cmake_minimum_required(VERSION 2.8)
project(pwnat)

include_directories(BEFORE ${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 ${SDL_CFLAGS}")

file(GLOB_RECURSE Sources pwnat/*.cpp)
add_executable(pwnat ${Sources})

