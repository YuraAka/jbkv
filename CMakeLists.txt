cmake_minimum_required(VERSION 3.5)

project(jbkv)
# Treat warnings as errors on other compilers
add_compile_options(-Werror)
set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0135 NEW)

# set(CMAKE_CACHEFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cache)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CACHEFILE_DIR ${CMAKE_BINARY_DIR}/cache)
# configure_file(CMakeLists.txt.in ${CMAKE_CACHEFILE_DIR}/CMakeLists.txt)

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Now simply link against gtest or gtest_main as needed. Eg
#add_executable(example example.cpp)

add_library(lib-jbkv STATIC
    lib/volume.cpp
    lib/volume.h
)

add_executable(example
  example/main.cpp
)

target_include_directories(example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
  example PRIVATE
  lib-jbkv
)

add_executable(unittest
  tests/unit.cpp
)

include_directories(${GTEST_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unittest ${GTEST_BOTH_LIBRARIES} lib-jbkv gtest_main)
target_compile_features(unittest PRIVATE cxx_std_11)

add_test(NAME example_test COMMAND unittest)
