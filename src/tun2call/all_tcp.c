
#include "all_tcp.h"
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
#include <stdlib.h>

static void all_tcp_pcb_free(struct all_tcp_pcb *es) {
  if (es != NULL) {
    if (es->sending) {
      /* free the buffer chain if present */
      pbuf_free(es->sending);
      es->sending = NULL;
    }
    if (es->recving) {
      /* free the buffer chain if present */
      pbuf_free(es->recving);
      es->recving = NULL;
    }
    free(es);
  }
}

static void all_tcp_close_all(struct all_tcp_pcb *es) {
  es->handler->close(es->handler, es);
  tcp_arg(es->raw, NULL);
  tcp_sent(es->raw, NULL);
  tcp_recv(es->raw, NULL);
  tcp_err(es->raw, NULL);
  tcp_poll(es->raw, NULL, 0);
  tcp_abort(es->raw);
  all_tcp_pcb_free(es);
}

void all_tcp_close(struct all_tcp_pcb *es) {
  es->state = ES_CLOSING;
}

void all_tcp_send(struct all_tcp_pcb *es) {
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;
  while ((wr_err == ERR_OK) && (es->sending != NULL) && (es->sending->len <= tcp_sndbuf(es->raw))) {
    ptr = es->sending;
    /* enqueue data for transmission */
    wr_err = tcp_write(es->raw, ptr->payload, ptr->len, 1);
    if (wr_err == ERR_OK) {
      /* continue with next pbuf in chain (if any) */
      es->sending = ptr->next;
      if (es->sending != NULL) {
        /* new reference! */
        pbuf_ref(es->sending);
      }
      es->handler->send(es->handler, es, ptr->len);
      /* chop first pbuf from chain */
      pbuf_free(ptr);
    } else if (wr_err == ERR_MEM) {
      /* we are low on memory, try later / harder, defer to poll */
      es->sending = ptr;
    }
  }
}

void all_tcp_send_buf(struct all_tcp_pcb *pcb, struct pbuf *buf) {
  if (pcb->sending) {
    pbuf_cat(pcb->sending, buf);
  } else {
    pcb->sending = buf;
  }
  all_tcp_send(pcb);
}

static void all_tcp_error(void *arg, err_t err) {
  LWIP_UNUSED_ARG(err);
  struct all_tcp_pcb *es = arg;
  es->handler->error(es->handler, es);
  all_tcp_pcb_free(es);
}

static err_t all_tcp_poll(void *arg, struct tcp_pcb *pcb) {
  err_t ret_err;
  struct all_tcp_pcb *es = arg;
  if (es != NULL) {
    if (es->sending != NULL) {
      /* there is a remaining pbuf (chain)  */
      all_tcp_send(es);
    } else if (es->state == ES_CLOSING) {
      all_tcp_close_all(es);
    } else {
      es->handler->poll(es->handler, es);
      if (es->sending != NULL) {
        all_tcp_send(es);
      }
    }
    ret_err = ERR_OK;
  } else {
    /* nothing to be done */
    tcp_abort(pcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

static err_t all_tcp_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
  LWIP_UNUSED_ARG(len);
  struct all_tcp_pcb *es = arg;
  if (es->sending != NULL) {
    tcp_sent(pcb, all_tcp_sent);
    all_tcp_send(es);
  } else if (es->state == ES_CLOSING) {
    all_tcp_close_all(es);
  }
  return ERR_OK;
}

static err_t all_tcp_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
  err_t ret_err;
  struct all_tcp_pcb *es = arg;
  if (p == NULL) {
    /* remote host closed connection */
    es->state = ES_CLOSING;
    if (es->sending == NULL) {
      /* we're done sending, close it */
      all_tcp_close_all(es);
    } else {
      /* we're not done yet */
      all_tcp_send(es);
    }
    ret_err = ERR_OK;
  } else if (err != ERR_OK) {
    /* cleanup, for unknown reason */
    LWIP_ASSERT("no pbuf expected here", p == NULL);
    ret_err = err;
  } else if (es->state == ES_ACCEPTED) {
    /* first data chunk in p->payload */
    es->state = ES_RECEIVED;
    /* store reference to incoming pbuf (chain) */
    LWIP_ASSERT("es->recving==NULL", es->recving == NULL);
    es->recving = p;
    ret_err = ERR_OK;
    es->handler->recv(es->handler, es);
  } else if (es->state == ES_RECEIVED) {
    /* read some more data */
    if (es->recving == NULL) {
      es->recving = p;
    } else {
      pbuf_cat(es->recving, p);
    }
    ret_err = ERR_OK;
    es->handler->recv(es->handler, es);
  } else {
    /* unknown es->state, trash data  */
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

static err_t all_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t recv_err) {
  if (recv_err != ERR_OK || (newpcb == NULL)) {
    return ERR_VAL;
  }
  struct all_tcp_pcb *es = (struct all_tcp_pcb *) malloc(sizeof(struct all_tcp_pcb));
  es->user = NULL;
  es->state = ES_ACCEPTED;
  es->mark = 0;
  es->handler = arg;
  es->raw = newpcb;
  es->sending = NULL;
  es->recving = NULL;
  /* pass newly allocated es to our callbacks */
  tcp_arg(newpcb, es);
  tcp_setprio(newpcb, TCP_PRIO_NORMAL);
  tcp_recv(newpcb, all_tcp_recv);
  tcp_err(newpcb, all_tcp_error);
  tcp_poll(newpcb, all_tcp_poll, 0);
  tcp_sent(newpcb, all_tcp_sent);
  es->handler->accept(es->handler, es);
  return ERR_OK;
}

err_t all_tcp_init(struct all_tcp_handler *handler) {
  handler->listener = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (handler->listener == NULL) {
    return ERR_MEM;
  }
  err_t err;
  err = tcp_bind(handler->listener, IP_ANY_TYPE, 7);
  if (err != ERR_OK) {
    tcp_close(handler->listener);
    handler->listener = NULL;
    return err;
  }
  handler->listener = tcp_listen(handler->listener);
  tcp_arg(handler->listener, handler);
  tcp_accept(handler->listener, all_tcp_accept);
  return ERR_OK;
}

err_t all_tcp_free(struct all_tcp_handler *handler) {
  err_t err = tcp_close(handler->listener);
  tcp_shutdown(handler->listener, 0, 0);
  handler->listener = 0;
  return err;
}

void all_tcp_select(struct all_tcp_handler *handler) {
  if (handler->select) {
    return handler->select(handler);
  }
}