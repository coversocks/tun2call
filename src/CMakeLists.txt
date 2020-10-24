set (tun2call_INCLUDE_DIRS
    "${LWIP_DIR}/src/include"
    "${LWIP_DIR}/contrib"
    "${LWIP_DIR}/contrib/ports/unix/port/include"
    "${LWIP_DIR}/contrib/examples/example_app"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/tun2call"
)
set(tun2call_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/tun2call/netif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tun2call/all_udp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tun2call/all_tcp.c
)
add_library(tun2call ${tun2call_SRCS})
add_dependencies(tun2call lwipcontribportunix lwipcore)
target_include_directories(tun2call PRIVATE ${tun2call_INCLUDE_DIRS})
install(TARGETS tun2call DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/tun2call DESTINATION include)