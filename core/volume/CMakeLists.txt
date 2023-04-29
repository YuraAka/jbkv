cmake_minimum_required(VERSION 3.5)
if(MSVC)
  # Treat warnings as errors on MSVC
  add_compile_options(/W4 /WX)
else()
  # Treat warnings as errors on other compilers
  add_compile_options(-Werror)
endif()

project(jbkv_volume)
set(CMAKE_CXX_STANDARD 20)
find_package(Protobuf REQUIRED)
set(SOURCES node.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${SOURCES})
include_directories(${PROTOBUF_INCLUDE_DIRS})
add_library(jbkv_volume STATIC
    ${SOURCES}
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    volume.cpp
    volume.h
)

target_link_libraries(protobuf::libprotobuf)

#target_link_libraries(mylib
#    dependency1
#    dependency2
#)

# Set the include directories
#target_include_directories(mylib
#    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
#)
