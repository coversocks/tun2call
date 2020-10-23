#ifndef NETIF_H
#define NETIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/ip.h"

typedef void (*netif_handler_init_fn)(void* user, struct netif* netif);
typedef ssize_t (*netif_handler_read_fn)(void* user, char** buf);
typedef ssize_t (*netif_handler_write_fn)(void* user, char* buf, u16_t len);

struct netif_handler {
  void* user;
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  netif_handler_init_fn init;
  netif_handler_read_fn read;
  netif_handler_write_fn write;
};

void netif_default_init(struct netif_handler* handler);
void netif_default_poll();
void netif_default_free();

#ifdef __cplusplus
}
#endif

#endif
