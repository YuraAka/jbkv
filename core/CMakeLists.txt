cmake_minimum_required(VERSION 3.0)
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake/common.txt)

project(core)
add_library(core STATIC
    ${SOURCES}
    volume.cpp
    volume.h
)
