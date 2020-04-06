cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(rres C)

# Config options
option(BUILD_RREM "Build rrem." ON)

add_subdirectory(src)

if (${BUILD_RREM})
  add_subdirectory(tools/rrem)
endif()

# TODO: Add automated testing
# enable_testing()