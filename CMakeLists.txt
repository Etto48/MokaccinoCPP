cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(Mokaccino)
file(GLOB_RECURSE sources "src/*.cpp")

set(CMAKE_COLOR_MAKEFILE ON)
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
set(Boost_DEBUG OFF)

find_package(Boost COMPONENTS thread chrono program_options REQUIRED)
add_compile_definitions(_WIN32_WINNT=0x0601)

include_directories(${Boost_INCLUDE_DIRS}) 
link_libraries(${Boost_LIBRARIES})
add_executable(Mokaccino ${sources})
