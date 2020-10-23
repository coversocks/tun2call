cmake_minimum_required(VERSION 3.10)

project(tun2call)

set(LWIP_DIR ${CMAKE_SOURCE_DIR}/lwip/)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(SEND_ERROR "In-source builds are not allowed.")
endif ()


add_subdirectory(lwip)
add_subdirectory(dns)

set (tun2call_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${LWIP_DIR}/contrib"
    "${LWIP_DIR}/contrib/ports/unix/port/include"
    "${LWIP_DIR}/contrib/examples/example_app"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/tun2call"
)
set(tun2call_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tun2call/netif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tun2call/all_udp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tun2call/all_tcp.c
)
add_library(tun2call ${tun2call_SRCS})
target_include_directories(tun2call PRIVATE ${tun2call_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include/)
install(TARGETS tun2call DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/tun2call DESTINATION include)

set (tun2echo_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/test"
)
set(tun2echo_SRCS
    ${CMAKE_SOURCE_DIR}/src/test/test.c
    ${CMAKE_SOURCE_DIR}/src/test/tapif.c
)
add_executable(tun2echo ${tun2echo_SRCS})
target_include_directories(tun2echo PRIVATE ${tun2call_INCLUDE_DIRS} ${tun2echo_INCLUDE_DIRS})
target_link_libraries(tun2echo tun2call lwipcore lwipcontribportunix lwipcore)