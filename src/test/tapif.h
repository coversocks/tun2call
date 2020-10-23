
#include "lwip/ip.h"
#include "netif/tapif.h"

void tapif_raw_init(void* user, struct netif* netif);
ssize_t tapif_raw_write(void* user, char* buf, u16_t len);
ssize_t tapif_raw_read(void* user, char** buf);