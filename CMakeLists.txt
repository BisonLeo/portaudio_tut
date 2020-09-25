cmake_minimum_required(VERSION 3.0.0)
project(pa_devs VERSION 0.1.0)

# include(CTest)
# enable_testing()


set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/portaudio/;${CMAKE_PREFIX_PATH}")

message(${CMAKE_PREFIX_PATH} "\nVerbose make ON")
#set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(portaudio)

add_executable(pa_devs pa_devs.c)
TARGET_LINK_LIBRARIES(pa_devs portaudio_static)

add_executable(sine paex_sine_c++.cpp
    ${CMAKE_SOURCE_DIR}/common/pa_ringbuffer.c)
include_directories(${CMAKE_SOURCE_DIR}/common)
# target_include_directories(pablio ${CMAKE_SOURCE_DIR}/common)
TARGET_LINK_LIBRARIES(sine portaudio_static)


# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)

