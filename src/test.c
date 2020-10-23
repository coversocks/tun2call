/* C runtime includes */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* lwIP core includes */
#include "lwip/api.h"
#include "lwip/autoip.h"
#include "lwip/debug.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/etharp.h"
#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/ip4_frag.h"
#include "lwip/netif.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "netif/ethernet.h"

/* applications includes */
#include "all_tcp.h"
#include "all_udp.h"
#include "netif.h"
#include "tapif.h"

/* include the port-dependent configuration */
#include "lwipcfg.h"

static struct netif_handler netif;
static struct all_tcp_handler tcp_all;
static struct all_udp_handler udp_all;

void all_tcp_handler_recv(struct all_tcp_handler* handler,
                          struct all_tcp_pcb* pcb) {
  if (pcb->recving) {
    tcp_recved(pcb->raw, pcb->recving->tot_len);
    all_tcp_send_buf(pcb, pcb->recving);
    pcb->recving = NULL;
  }
}

void all_tcp_handler_poll(struct all_tcp_handler* handler,
                          struct all_tcp_pcb* pcb) {
  // printf("poll--->\n");
}

void all_tcp_handler_close(struct all_tcp_handler* handler,
                           struct all_tcp_pcb* pcb) {
  // printf("close--->\n");
}

void all_tcp_handler_accept(struct all_tcp_handler* handler,
                            struct all_tcp_pcb* pcb) {
  // printf("accpet--->\n");
}

void all_udp_handler_recv(struct all_udp_handler* handler,
                          struct udp_pcb* pcb,
                          const ip_addr_t* remote_addr,
                          u16_t remote_port,
                          struct pbuf* p) {
  all_udp_sendto(handler, &pcb->local_ip, pcb->local_port, remote_addr,
                 remote_port, p);
  pbuf_free(p);
}
void all_udp_handler_poll(struct all_udp_handler* handler,
                          struct udp_pcb* pcb) {
  // printf("poll--->\n");
}

/* This function initializes this lwIP test. When NO_SYS=1, this is done in
 * the main_loop context (there is no other one), when NO_SYS=0, this is done
 * in the tcpip_thread context */
static void test_init(void* arg) { /* remove compiler warning */
  LWIP_UNUSED_ARG(arg);
  /* init randomizer again (seed per thread) */
  srand((unsigned int)time(0));

  /* init network interfaces */
  ip4_addr_set_zero(&netif.gw);
  ip4_addr_set_zero(&netif.ipaddr);
  ip4_addr_set_zero(&netif.netmask);
  LWIP_PORT_INIT_GW(&netif.gw);
  LWIP_PORT_INIT_IPADDR(&netif.ipaddr);
  LWIP_PORT_INIT_NETMASK(&netif.netmask);
  netif.init = tapif_raw_init;
  netif.read = tapif_raw_read;
  netif.write = tapif_raw_write;
  netif_default_init(&netif);

  /* init apps */
  tcp_all.recv = all_tcp_handler_recv;
  tcp_all.poll = all_tcp_handler_poll;
  tcp_all.close = all_tcp_handler_close;
  tcp_all.accept = all_tcp_handler_accept;
  all_tcp_init(&tcp_all);
  udp_all.recv = all_udp_handler_recv;
  udp_all.poll = all_udp_handler_poll;
  all_udp_init(&udp_all);
}

int main(void) {
  /* initialize lwIP stack, network interfaces and applications */
  lwip_init();
  test_init(NULL);
  /* MAIN LOOP for driver update (and timers if NO_SYS) */
  while (1) {
    netif_default_poll();
    all_udp_poll(&udp_all);
  }
  netif_default_free();
  return 0;
}

/* This function is only required to prevent arch.h including stdio.h
 * (which it does if LWIP_PLATFORM_ASSERT is undefined)
 */
void lwip_example_app_platform_assert(const char* msg,
                                      int line,
                                      const char* file) {
  printf("Assertion \"%s\" failed at line %d in %s\n", msg, line, file);
  fflush(NULL);
  abort();
}
