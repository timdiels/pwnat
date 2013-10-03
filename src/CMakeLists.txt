cmake_minimum_required(VERSION 2.8)
project(pwnat)

set(Boost_USE_STATIC_LIBS OFF) 
find_package(Boost REQUIRED COMPONENTS system) 

if(!Boost_FOUND)
    message("Boost not found")
endif()

include_directories(BEFORE ${CMAKE_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS}) 

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 ${SDL_CFLAGS}")

file(GLOB_RECURSE Sources pwnat/*.cpp)
add_executable(pwnat ${Sources})
target_link_libraries(pwnat ${Boost_LIBRARIES})                                                                                                                                                                                                                            

