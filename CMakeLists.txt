cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(Mokaccino)
file(GLOB_RECURSE sources "src/*.cpp")
file(GLOB_RECURSE tests "tests/*.cpp")

#options
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
set(Boost_DEBUG OFF)

#packages
find_package(Boost COMPONENTS thread chrono program_options REQUIRED)
find_package(portaudio REQUIRED)
find_package(Opus REQUIRED)

#debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
endif()

#definitions
add_compile_definitions(_WIN32_WINNT=0x0601)
option(DISABLE_ANSI_ESCAPE "If enabled the output text won't use ANSI escape sequences" OFF)
if(DISABLE_ANSI_ESCAPE)
    message(STATUS "Disabling output colors")
    add_compile_definitions(NO_ANSI_ESCAPE)
endif()

#link and include
include_directories(${Boost_INCLUDE_DIRS}) 
link_libraries(${Boost_LIBRARIES} portaudio_static Opus::opus)
add_executable(Mokaccino ${sources})

#testing
enable_testing()
set(STARTING_TEST_PORT 23235)
foreach(test IN ITEMS ${tests})
    get_filename_component(TEST_NAME ${test} NAME_WE)
    message(STATUS "Adding test " ${TEST_NAME})
    add_executable(test-${TEST_NAME} ${sources} ${test})
    target_compile_definitions(test-${TEST_NAME} PUBLIC _TEST PUBLIC NO_ANSI_ESCAPE)
    target_include_directories(test-${TEST_NAME} PUBLIC "src")
    add_test(NAME test-${TEST_NAME} COMMAND $<TARGET_FILE:test-${TEST_NAME}> --config ${CMAKE_SOURCE_DIR}/example.cfg --port ${STARTING_TEST_PORT} --username "mokaccino")
    math(EXPR STARTING_TEST_PORT "${STARTING_TEST_PORT}+1" OUTPUT_FORMAT DECIMAL)
    set(TESTS ${TESTS} test-${TEST_NAME})
endforeach()
set_tests_properties(${TESTS} PROPERTIES TIMEOUT 10)


