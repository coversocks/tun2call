cmake_minimum_required(VERSION 3.10)

project(tun2call)

set(LWIP_DIR ${CMAKE_SOURCE_DIR}/lwip/)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif ()


add_subdirectory(lwip)
add_subdirectory(dns)

set (LWIP_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${LWIP_DIR}/contrib"
    "${LWIP_DIR}/contrib/ports/unix/port/include"
    "${LWIP_DIR}/contrib/examples/example_app"
)

set(tun2call_SRCS
    ${CMAKE_SOURCE_DIR}/src/netif.c
    ${CMAKE_SOURCE_DIR}/src/all_udp.c
    ${CMAKE_SOURCE_DIR}/src/all_tcp.c
)
add_library(tun2call EXCLUDE_FROM_ALL ${tun2call_SRCS})
target_include_directories(tun2call PRIVATE ${LWIP_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src/)

set(tun2echo_SRCS
    ${CMAKE_SOURCE_DIR}/src/test.c
    ${CMAKE_SOURCE_DIR}/src/tapif.c
)
add_executable(tun2conn_test EXCLUDE_FROM_ALL ${tun2echo_SRCS})
target_include_directories(tun2echo PRIVATE ${LWIP_INCLUDE_DIRS})
target_link_libraries(tun2echo tun2call lwipcore lwipcontribportunix lwipcore tun2call)