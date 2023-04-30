cmake_minimum_required(VERSION 3.0)
include(${CMAKE_CURRENT_SOURCE_DIR}/../CMake/common.txt)

project(core)
find_package(Protobuf REQUIRED)
set(SOURCES node.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${SOURCES})
include_directories(${PROTOBUF_INCLUDE_DIRS})
add_library(core STATIC
    ${SOURCES}
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    volume.cpp
    volume.h
)

target_link_libraries(protobuf::libprotobuf)
