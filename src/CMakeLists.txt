add_library(rres INTERFACE)

file(GLOB sources *.h)
set(RRES_HEADERS, ${sources})

install(FILES
  ${RRES_HEADERS} DESTINATION include/
)

target_include_directories(rres INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
