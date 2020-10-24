
#include "lwip/ip.h"
#include "netif/tapif.h"

void tapif_raw_init(struct netif_handler* handler, struct netif* netif);
ssize_t tapif_raw_write(struct netif_handler* handler);
ssize_t tapif_raw_read(struct netif_handler* handler);