#include "all_udp.h"

static void all_udp_recv(void* arg,
                         struct udp_pcb* pcb,
                         struct pbuf* p,
                         const ip_addr_t* addr,
                         u16_t port) {
  struct all_udp_handler* handler = arg;
  if (p != NULL) {
    handler->recv(handler, pcb, addr, port, p);
  }
}

err_t all_udp_init(struct all_udp_handler* handler) {
  err_t err;
  handler->listener = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (handler->listener == NULL) {
    return ERR_MEM;
  }
  err = udp_bind(handler->listener, IP_ANY_TYPE, 0);
  if (err != ERR_OK) {
    udp_remove(handler->listener);
    handler->listener = NULL;
    return err;
  }
  udp_recv(handler->listener, all_udp_recv, handler);
  return ERR_OK;
}

void all_udp_free(struct all_udp_handler* handler){
  udp_remove(handler->listener);
  handler->listener = 0;
}

err_t all_udp_poll(struct all_udp_handler* handler) {
  if (handler->poll) {
    handler->poll(handler, handler->listener);
  }
  return ERR_OK;
}

static struct netif* all_udp_get_current_netif(struct udp_pcb* pcb,
                                               const ip_addr_t* dst_ip,
                                               u16_t dst_port) {
  struct netif* netif;
  LWIP_UNUSED_ARG(dst_port);

  if (pcb->netif_idx != NETIF_NO_INDEX) {
    netif = netif_get_by_index(pcb->netif_idx);
  } else {
#if LWIP_MULTICAST_TX_OPTIONS
    netif = NULL;
    if (ip_addr_ismulticast(dst_ip)) {
      /* For IPv6, the interface to use for packets with a multicast destination
       * is specified using an interface index. The same approach may be used
       * for IPv4 as well, in which case it overrides the IPv4 multicast
       * override address below. Here we have to look up the netif by going
       * through the list, but by doing so we skip a route lookup. If the
       * interface index has gone stale, we fall through and do the regular
       * route lookup after all. */
      if (pcb->mcast_ifindex != NETIF_NO_INDEX) {
        netif = netif_get_by_index(pcb->mcast_ifindex);
      }
#if LWIP_IPV4
      else
#if LWIP_IPV6
          if (IP_IS_V4(dst_ip))
#endif /* LWIP_IPV6 */
      {
        /* IPv4 does not use source-based routing by default, so we use an
             administratively selected interface for multicast by default.
             However, this can be overridden by setting an interface address
             in pcb->mcast_ip4 that is used for routing. If this routing lookup
             fails, we try regular routing as though no override was set. */
        if (!ip4_addr_isany_val(pcb->mcast_ip4) &&
            !ip4_addr_cmp(&pcb->mcast_ip4, IP4_ADDR_BROADCAST)) {
          netif = ip4_route_src(ip_2_ip4(&pcb->local_ip), &pcb->mcast_ip4);
        }
      }
#endif /* LWIP_IPV4 */
    }

    if (netif == NULL)
#endif /* LWIP_MULTICAST_TX_OPTIONS */
    {
      /* find the outgoing network interface for this packet */
      netif = ip_route(&pcb->local_ip, dst_ip);
    }
  }
  return netif;
}

err_t all_udp_sendto(struct all_udp_handler* handler,
                     const ip_addr_t* local_addr,
                     u16_t local_port,
                     const ip_addr_t* remote_addr,
                     u16_t remote_port,
                     struct pbuf* p) {
  ip_addr_t old_addr = handler->listener->local_ip;
  u16_t old_port = handler->listener->local_port;
  handler->listener->local_ip = *local_addr;
  handler->listener->local_port = local_port;
  struct netif* netif =
      all_udp_get_current_netif(handler->listener, remote_addr, remote_port);
  err_t err = udp_sendto_if_src(handler->listener, p, remote_addr, remote_port,
                                netif, &handler->listener->local_ip);
  handler->listener->local_ip = old_addr;
  handler->listener->local_port = old_port;
  return err;
}