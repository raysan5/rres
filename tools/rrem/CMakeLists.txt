add_executable(rrem
	src/rrem.c
	src/external/lz4.c
	src/external/miniz.c
	src/external/tinyfiledialogs.c)

find_package(Raylib)

target_link_libraries(rrem rres raylib)

target_include_directories(rrem PUBLIC
    ${raylib_SOURCE_DIR}/src
    src/external
)