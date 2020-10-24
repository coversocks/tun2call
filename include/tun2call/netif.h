#ifndef NETIF_H
#define NETIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/ip.h"
#include "lwip/pbuf.h"

struct netif_handler;
typedef void (*netif_handler_init_fn)(struct netif_handler* handler, struct netif* netif);
typedef ssize_t (*netif_handler_read_fn)(struct netif_handler* handler);
typedef ssize_t (*netif_handler_write_fn)(struct netif_handler* handler);

struct netif_handler {
  void* user;
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  char *rbuf;
  char *wbuf;
  u16_t wbuf_size;
  netif_handler_init_fn init;
  netif_handler_read_fn read;
  netif_handler_write_fn write;
};
void netif_handler_set(struct netif_handler* handler,
                       u32_t ipaddr,
                       u32_t netmask,
                       u32_t gw);
void netif_default_init(struct netif_handler* handler);
void netif_default_poll();
void netif_default_free();

#ifdef __cplusplus
}
#endif

#endif
