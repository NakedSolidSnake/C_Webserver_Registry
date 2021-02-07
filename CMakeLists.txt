cmake_minimum_required(VERSION 3.7.0)
project(Webserver)

set (CMAKE_C_FLAGS "-D_GNU_SOURCE -Wall -std=c99")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_BUILD_TYPE Debug)

# build webserver
add_executable(webserver "")
# target_link_libraries(webserver PUBLIC ulfius ipc orcania)
target_link_libraries(webserver PUBLIC ulfius orcania)

target_sources(webserver
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

install(TARGETS webserver DESTINATION bin)
file (COPY assets DESTINATION bin)
