/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/ethip6.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"

#include "tun2call/netif.h"
#include "tapif.h"

#define IFCONFIG_BIN "/sbin/ifconfig "

#if defined(LWIP_UNIX_LINUX)
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
/*
 * Creating a tap interface requires special privileges. If the interfaces
 * is created in advance with `tunctl -u <user>` it can be opened as a regular
 * user. The network must already be configured. If DEVTAP_IF is defined it
 * will be opened instead of creating a new tap device.
 *
 * You can also use PRECONFIGURED_TAPIF environment variable to do so.
 */
#ifndef DEVTAP_DEFAULT_IF
#define DEVTAP_DEFAULT_IF "tap0"
#endif
#ifndef DEVTAP
#define DEVTAP "/dev/net/tun"
#endif
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d " NETMASK_ARGS
#elif defined(LWIP_UNIX_OPENBSD)
#define DEVTAP "/dev/tun0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tun0 inet %d.%d.%d.%d " NETMASK_ARGS " link0"
#else /* others */
#define DEVTAP "/dev/tap0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d " NETMASK_ARGS
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 't'
#define IFNAME1 'p'

#ifndef TAPIF_DEBUG
#define TAPIF_DEBUG LWIP_DBG_OFF
#endif

/*-----------------------------------------------------------------------------------*/
void tapif_raw_init(struct netif_handler* handler, struct netif* netif) {
  int* tapif;
#if LWIP_IPV4
  int ret;
  char buf[1024];
#endif /* LWIP_IPV4 */
  char* preconfigured_tapif = getenv("PRECONFIGURED_TAPIF");
  tapif = (int*)mem_malloc(sizeof(int));
  *tapif = open(DEVTAP, O_RDWR);
  LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_init: fd %d\n", tapif->fd));
  if (*tapif == -1) {
#ifdef LWIP_UNIX_LINUX
    perror(
        "tapif_init: try running \"modprobe tun\" or rebuilding your kernel "
        "with CONFIG_TUN; cannot open " DEVTAP);
#else  /* LWIP_UNIX_LINUX */
    perror("tapif_init: cannot open " DEVTAP);
#endif /* LWIP_UNIX_LINUX */
    exit(1);
  }
  handler->user = tapif;
#ifdef LWIP_UNIX_LINUX
  {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    if (preconfigured_tapif) {
      strncpy(ifr.ifr_name, preconfigured_tapif, sizeof(ifr.ifr_name));
    } else {
      strncpy(ifr.ifr_name, DEVTAP_DEFAULT_IF, sizeof(ifr.ifr_name));
    }
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = 0; /* ensure \0 termination */

    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (ioctl(*tapif, TUNSETIFF, (void*)&ifr) < 0) {
      perror("tapif_init: " DEVTAP " ioctl TUNSETIFF");
      exit(1);
    }
  }
#endif /* LWIP_UNIX_LINUX */

  if (preconfigured_tapif == NULL) {
#if LWIP_IPV4
    snprintf(buf, 1024, IFCONFIG_BIN IFCONFIG_ARGS,
             ip4_addr1(netif_ip4_gw(netif)), ip4_addr2(netif_ip4_gw(netif)),
             ip4_addr3(netif_ip4_gw(netif)), ip4_addr4(netif_ip4_gw(netif))
#ifdef NETMASK_ARGS
                                                 ,
             ip4_addr1(netif_ip4_netmask(netif)),
             ip4_addr2(netif_ip4_netmask(netif)),
             ip4_addr3(netif_ip4_netmask(netif)),
             ip4_addr4(netif_ip4_netmask(netif))
#endif /* NETMASK_ARGS */
    );

    LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_init: system(\"%s\");\n", buf));
    ret = system(buf);
    if (ret < 0) {
      perror("ifconfig failed");
      exit(1);
    }
    if (ret != 0) {
      printf("ifconfig returned %d\n", ret);
    }
#else  /* LWIP_IPV4 */
    perror("todo: support IPv6 support for non-preconfigured tapif");
    exit(1);
#endif /* LWIP_IPV4 */
  }
}

ssize_t tapif_raw_write(struct netif_handler* handler) {
  int* tapif = handler->user;
  ssize_t n = write(*tapif, handler->wbuf, handler->wbuf_size);
  return n;
}

ssize_t tapif_raw_read(struct netif_handler* handler) {
  int* tapif = handler->user;
  ssize_t n = read(*tapif, handler->rbuf, handler->rbuf_cap);
  if(n>=0){
    handler->rbuf_size=n;
  }else{
    handler->rbuf_size=0;
  }
  return n;
}