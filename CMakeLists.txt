cmake_minimum_required(VERSION 3.5)

project(jbkv)
if(MSVC)
  # Treat warnings as errors on MSVC
  add_compile_options(/W4 /WX)
else()
  # Treat warnings as errors on other compilers
  add_compile_options(-Werror)
endif()
set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_CACHEFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cache)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CACHEFILE_DIR ${CMAKE_BINARY_DIR}/cache)
# configure_file(CMakeLists.txt.in ${CMAKE_CACHEFILE_DIR}/CMakeLists.txt)

add_library(lib-jbkv STATIC
    lib/volume.cpp
    lib/volume.h
)

add_executable(example
  example/main.cpp
)

target_include_directories(example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

target_link_libraries(example PRIVATE lib-jbkv)
