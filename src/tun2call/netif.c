
#include "netif.h"
#include "lwip/api.h"
#include "lwip/autoip.h"
#include "lwip/debug.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/ip4_frag.h"
#include "lwip/netif.h"
#include "lwip/opt.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "netif/ethernet.h"

static err_t netif_default_output(struct netif* netif, struct pbuf* p) {
  struct netif_handler* handler = (struct netif_handler*)netif->state;
  char buf[1518]; /* max packet size including VLAN excluding CRC */
  if (p->tot_len > sizeof(buf)) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("netif_default_output: packet too large");
    return ERR_IF;
  }
  /* initiate transfer(); */
  pbuf_copy_partial(p, buf, p->tot_len, 0);
  /* signal that packet should be sent(); */
  ssize_t written = handler->write(handler->user, buf, p->tot_len);
  if (written < p->tot_len) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("netif_default_output: write");
    return ERR_IF;
  } else {
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, (u32_t)written);
    return ERR_OK;
  }
}

static struct pbuf* netif_default_read(struct netif* netif) {
  struct netif_handler* handler = (struct netif_handler*)netif->state;
  char* buf; /* max packet size including VLAN excluding CRC */
  ssize_t readlen = handler->read(handler->user, &buf);
  if (readlen < 0) {
    return 0;
  }
  u16_t len = (u16_t)readlen;
  MIB2_STATS_NETIF_ADD(netif, ifinoctets, len);
  /* We allocate a pbuf chain of pbufs from the pool. */
  struct pbuf* p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  if (p != NULL) {
    pbuf_take(p, buf, len);
    /* acknowledge that packet has been read(); */
  } else {
    /* drop packet(); */
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    perror("netif_default_input: could not allocate pbuf\n");
  }
  return p;
}

static void netif_default_input(struct netif* netif) {
  struct pbuf* p = netif_default_read(netif);
  if (p == NULL) {
    return;
  }
  if (netif->input(p, netif) != ERR_OK) {
    pbuf_free(p);
  }
}

static err_t netif_default_low_init(struct netif* netif) {
  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);
  netif->name[0] = 't';
  netif->name[1] = 'p';
  netif->output = etharp_output;
  netif->output_ip6 = ethip6_output;
  netif->linkoutput = netif_default_output;
  netif->mtu = 1500;
  netif->hwaddr[0] = 0x02;
  netif->hwaddr[1] = 0x12;
  netif->hwaddr[2] = 0x34;
  netif->hwaddr[3] = 0x56;
  netif->hwaddr[4] = 0x78;
  netif->hwaddr[5] = 0xab;
  netif->hwaddr_len = 6;
  /* device capabilities */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
  struct netif_handler* handler = netif->state;
  handler->init(handler->user, netif);
  netif_set_link_up(netif);
  return ERR_OK;
}

static struct netif netif_;

/* globales variables for netifs */
static void netif_default_status_callback(struct netif* netif) {
  if (netif_is_up(netif)) {
    printf("Starting lwIP, status is UP, local interface IP is %s\n",
           ip4addr_ntoa(netif_ip4_addr(netif)));
  } else {
    printf("Starting lwIP, status is DOWN\n");
  }
}

static void netif_default_link_callback(struct netif* netif) {
  if (netif_is_link_up(netif)) {
    printf("Starting lwIP, link is UP\n");
  } else {
    printf("Starting lwIP, link is DOWN\n");
  }
}

/* This function initializes all network interfaces */
void netif_default_init(struct netif_handler* handler) {
  printf("Starting lwIP, local interface IP is %s\n",
         ip4addr_ntoa(&handler->ipaddr));
  netif_add(&netif_, &handler->ipaddr, &handler->netmask, &handler->gw, handler,
            netif_default_low_init, netif_input);
  netif_set_default(&netif_);
  netif_create_ip6_linklocal_address(&netif_, 1);
  printf("Starting lwIP, ip6 linklocal address is %s\n",
         ip6addr_ntoa(netif_ip6_addr(&netif_, 0)));
  netif_set_status_callback(&netif_, netif_default_status_callback);
  netif_set_link_callback(&netif_, netif_default_link_callback);
  netif_set_up(&netif_);
}

void netif_default_poll() {
  /* handle timers (already done in tcpip.c when NO_SYS=0) */
  sys_check_timeouts();
  netif_default_input(&netif_);
  /* check for loopback packets on all netifs */
  netif_poll_all();
}

void netif_default_free() {
  netif_set_down(&netif_);
}