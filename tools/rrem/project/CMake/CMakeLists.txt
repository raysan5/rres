cmake_minimum_required(VERSION 3.11)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
project(rrem C)

set(RREM_DIR ../../src)
set(RRES_DIR ../../../../src)

add_executable(rrem
	${RREM_DIR}/rrem.c
	${RREM_DIR}/external/lz4.c
	${RREM_DIR}/external/miniz.c
	${RREM_DIR}/external/tinyfiledialogs.c
)

find_package(Raylib)

target_link_libraries(rrem raylib)

target_include_directories(rrem PUBLIC
    ${raylib_SOURCE_DIR}/src
    ${RREM_DIR}
    ${RREM_DIR}/external
    ${RRES_DIR}
)