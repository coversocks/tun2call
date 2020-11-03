
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

#if __ANDROID__

#include <android/log.h>
#define LOG_TAG "coversocks"
#define LOG_DEBUG(...)                                                         \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...)                                                         \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#else

#define LOG_DEBUG(...) printf(__VA_ARGS__)
#define LOG_ERROR(...) perror(__VA_ARGS__)

#endif

static err_t netif_default_output(struct netif *netif, struct pbuf *p) {
  struct netif_handler *handler = (struct netif_handler *)netif->state;
  /* signal that packet should be sent(); */
  ssize_t written = handler->write(handler, p);
  if (written < p->tot_len) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    LOG_ERROR("netif_default_output: write");
    return ERR_IF;
  } else {
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, (u32_t)written);
    return ERR_OK;
  }
}

static struct pbuf *netif_default_read(struct netif *netif) {
  struct netif_handler *handler = (struct netif_handler *)netif->state;
  struct pbuf *p = handler->read(handler);
  return p;
}

static void netif_default_input(struct netif *netif) {
  struct pbuf *p = netif_default_read(netif);
  if (p == NULL) {
    return;
  }
  if (netif->input(p, netif) != ERR_OK) {
    pbuf_free(p);
  }
}

static err_t netif_default_low_init(struct netif *netif) {
  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);
  netif->name[0] = 't';
  netif->name[1] = 'p';
  netif->output = netif_default_output;
  netif->output_ip6 = netif_default_output;
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
  struct netif_handler *handler = netif->state;
  handler->init(handler, netif);
  netif_set_link_up(netif);
  return ERR_OK;
}

static struct netif *default_ = 0;

/* globales variables for netifs */
static void netif_default_status_callback(struct netif *netif) {
  if (netif_is_up(netif)) {
    LOG_DEBUG("Starting lwIP, status is UP, local interface IP is %s\n",
              ip4addr_ntoa(netif_ip4_addr(netif)));
  } else {
    LOG_DEBUG("Starting lwIP, status is DOWN\n");
  }
}

static void netif_default_link_callback(struct netif *netif) {
  if (netif_is_link_up(netif)) {
    LOG_DEBUG("Starting lwIP, link is UP\n");
  } else {
    LOG_DEBUG("Starting lwIP, link is DOWN\n");
  }
}

void netif_handler_set(struct netif_handler *handler, u32_t ipaddr,
                       u32_t netmask, u32_t gw) {
  handler->ipaddr.addr = PP_HTONL(ipaddr);
  handler->netmask.addr = PP_HTONL(netmask);
  handler->gw.addr = PP_HTONL(gw);
}

/* This function initializes all network interfaces */
void netif_default_init(struct netif_handler *handler) {
  LOG_DEBUG("Starting lwIP, local interface IP is %s\n",
            ip4addr_ntoa(&handler->ipaddr));
  default_ = malloc(sizeof(struct netif));
  netif_add(default_, &handler->ipaddr, &handler->netmask, &handler->gw,
            handler, netif_default_low_init, netif_input);
  if (handler->enable_ipv6) {
    netif_create_ip6_linklocal_address(default_, 1);
    printf("Starting lwIP, ip6 linklocal address is %s\n",
           ip6addr_ntoa(netif_ip6_addr(default_, 0)));
  }
  netif_set_status_callback(default_, netif_default_status_callback);
  netif_set_link_callback(default_, netif_default_link_callback);
  netif_set_default(default_);
  netif_set_up(default_);
}

void netif_default_poll() {
  /* handle timers (already done in tcpip.c when NO_SYS=0) */
  sys_check_timeouts();
  netif_default_input(default_);
  /* check for loopback packets on all netifs */
  netif_poll_all();
}

void netif_default_free() {
  netif_set_down(default_);
  netif_remove(default_);
  free(default_);
  default_ = 0;
}

void lwip_platform_assert(const char *msg, int line, const char *file) {
  LOG_ERROR("Assertion \"%s\" failed at line %d in %s\n", msg, line, file);
  fflush(NULL);
  abort();
}
