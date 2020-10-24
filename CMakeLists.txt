cmake_minimum_required(VERSION 3.10)

project(tun2call)

set(LWIP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lwip/)

if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif ()


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lwip)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dns)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/tun2call)

set (tun2echo_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/test"
)
set(tun2echo_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test/tapif.c
)
add_executable(tun2echo ${tun2echo_SRCS})
target_include_directories(tun2echo PRIVATE ${tun2call_INCLUDE_DIRS} ${tun2echo_INCLUDE_DIRS})
target_link_libraries(tun2echo tun2call lwipcore lwipcontribportunix lwipcore)