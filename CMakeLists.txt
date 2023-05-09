cmake_minimum_required(VERSION 3.5)

project(jbkv)
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Werror)
endif()

set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0135 NEW)

# set(CMAKE_CACHEFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cache)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CACHEFILE_DIR ${CMAKE_BINARY_DIR}/cache)

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${GTEST_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lib-jbkv STATIC
    lib/volume.cpp
    lib/volume.h
)

add_executable(unittest tests/unit.cpp)
target_link_libraries(unittest ${GTEST_BOTH_LIBRARIES} lib-jbkv gtest_main)
target_compile_features(unittest PRIVATE cxx_std_11)

add_executable(functest tests/func.cpp)
target_link_libraries(functest ${GTEST_BOTH_LIBRARIES} lib-jbkv gtest_main)
target_compile_features(functest PRIVATE cxx_std_11)

add_executable(stresstest tests/stress.cpp)
target_link_libraries(stresstest ${GTEST_BOTH_LIBRARIES} lib-jbkv gtest_main)
target_compile_features(stresstest PRIVATE cxx_std_11)
