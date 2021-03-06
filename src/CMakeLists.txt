cmake_minimum_required(VERSION 2.8)
project(pwnat)

add_definitions(-DBOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS OFF) 
find_package(Boost COMPONENTS system thread program_options log)
if(!Boost_FOUND)
    message("Boost not found")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
find_package(UDT REQUIRED)
if (!UDT_FOUND)
    message("UDT not found")
endif()


include_directories(BEFORE ${CMAKE_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS} ${UDT_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 ${SDL_CFLAGS}")

file(GLOB_RECURSE Sources pwnat/*.cpp)
add_executable(pwnat ${Sources})
target_link_libraries(pwnat ${Boost_LIBRARIES} ${UDT_LIBRARIES})

