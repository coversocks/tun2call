#ifndef ALL_UDP_H
#define ALL_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/ip.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

struct all_udp_handler;

typedef void (*all_udp_handler_recv_fn)(struct all_udp_handler* handler,
                                        struct udp_pcb* pcb,
                                        const ip_addr_t* remote_addr,
                                        u16_t remote_port,
                                        struct pbuf* p);
typedef void (*all_udp_handler_poll_fn)(struct all_udp_handler* handler,
                                        struct udp_pcb* pcb);
struct all_udp_handler {
  void *user;
  struct udp_pcb* listener;
  all_udp_handler_recv_fn recv;
  all_udp_handler_poll_fn poll;
};

err_t all_udp_init(struct all_udp_handler* handler);
err_t all_udp_poll(struct all_udp_handler* handler);
err_t all_udp_sendto(struct all_udp_handler* handler,
                     const ip_addr_t* local_addr,
                     u16_t local_port,
                     const ip_addr_t* remote_addr,
                     u16_t remote_port,
                     struct pbuf* p);

#ifdef __cplusplus
}
#endif

#endif