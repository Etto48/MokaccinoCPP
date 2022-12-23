cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(Mokaccino)
file(GLOB_RECURSE sources "src/*.cpp")
file(GLOB_RECURSE tests "tests/*.cpp")

#options
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
set(Boost_DEBUG OFF)
set(OPENSSL_USE_STATIC_LIBS OFF)

#packages
find_package(Boost COMPONENTS thread chrono program_options REQUIRED)
find_package(portaudio REQUIRED)
find_package(Opus REQUIRED)
find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_libraries(OpenSSL::Crypto)
else()
    add_compile_definitions(NO_OPENSSL)
endif()

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
option(DISABLE_TERMINAL_UI "If enabled the output text won't use ANSI to behave like a terminal user interface" OFF)
if(DISABLE_TERMINAL_UI OR DISABLE_ANSI_ESCAPE)
    message(STATUS "Disabling terminal UI")
    add_compile_definitions(NO_TERMINAL_UI)
endif()

option(DISABLE_CURSES "If enabled then PDCurses or NCurses will not be used" OFF)
if(NOT DISABLE_CURSES)
    if(WIN32)
        set(PDCurses_INCLUDE_DIRS "C:/Program Files/PDCurses/include")
        set(PDCurses_LIBRARIES "C:/Program Files/PDCurses/lib/PDCurses.lib")
        if(EXISTS ${PDCurses_LIBRARIES})
            include_directories(${PDCurses_INCLUDE_DIRS})
            link_libraries(${PDCurses_LIBRARIES})
            message(STATUS "Found PDCurses at \"C:/Program Files/PDCurses\"")
        else()
            message(STATUS "Could NOT find PDCurses")
            add_compile_definitions(NO_CURSES)
        endif()
    else()
        find_package(Curses)
        if(CURSES_FOUND)
            include_directories(${CURSES_INCLUDE_DIRS})
            link_libraries(${CURSES_LIBRARIES})
        else()
            add_compile_definitions(NO_CURSES)
        endif()

    endif()
else()
    message(STATUS "Disabling CURSES")
    add_compile_definitions(NO_CURSES)
endif()

option(DISABLE_CLOCK "If enabled the output will no more include a clock" OFF)
if(DISABLE_CLOCK)
    add_compile_definitions(NO_CLOCK)
    message(STATUS "Disabling clock")
endif()
#git tag as version
execute_process(COMMAND git describe --tags
    TIMEOUT 5
    OUTPUT_VARIABLE GIT_TAG_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(GIT_TAG_VERSION)
    message(STATUS "Building with git tag ${GIT_TAG_VERSION}")
    add_compile_definitions(BUILD_TAG_VERSION=\"${GIT_TAG_VERSION}\")
else()
    message(WARNING "Error finding git tag, defaulting to version v1.0.0")
    add_compile_definitions(BUILD_TAG_VERSION=\"v1.0.0\")
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
    target_compile_definitions(test-${TEST_NAME} PUBLIC _TEST PUBLIC NO_ANSI_ESCAPE NO_CURSES)
    target_include_directories(test-${TEST_NAME} PUBLIC "src")
    add_test(NAME test-${TEST_NAME} COMMAND $<TARGET_FILE:test-${TEST_NAME}> --port ${STARTING_TEST_PORT})
    math(EXPR STARTING_TEST_PORT "${STARTING_TEST_PORT}+1" OUTPUT_FORMAT DECIMAL)
    set(TESTS ${TESTS} test-${TEST_NAME})
endforeach()
set_tests_properties(${TESTS} PROPERTIES TIMEOUT 10)


