cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(Mokaccino)
file(GLOB_RECURSE sources "src/*.cpp")

#options
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
set(Boost_DEBUG OFF)

#packages
find_package(Boost COMPONENTS thread chrono program_options REQUIRED)
find_package(portaudio REQUIRED)

#definitions
add_compile_definitions(_WIN32_WINNT=0x0601)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

#link and include
include_directories(${Boost_INCLUDE_DIRS}) 
link_libraries(${Boost_LIBRARIES} portaudio_static)
add_executable(Mokaccino ${sources})