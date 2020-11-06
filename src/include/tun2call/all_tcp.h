#ifndef ALL_TCP_H
#define ALL_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

struct all_tcp_handler;

enum all_tcp_states {
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

struct all_tcp_pcb {
  void *user;
  u8_t state;
  struct tcp_pcb *raw;
  struct pbuf *recving;
  struct pbuf *sending;
  struct all_tcp_handler *handler;
};

typedef void (*all_tcp_select_fn)(struct all_tcp_handler *handler);

typedef void (*all_tcp_recv_fn)(struct all_tcp_handler *handler, struct all_tcp_pcb *pcb);

typedef void (*all_tcp_poll_fn)(struct all_tcp_handler *handler, struct all_tcp_pcb *pcb);

typedef void (*all_tcp_close_fn)(struct all_tcp_handler *handler, struct all_tcp_pcb *pcb);

typedef void (*all_tcp_accept_fn)(struct all_tcp_handler *handler, struct all_tcp_pcb *pcb);

struct all_tcp_handler {
  void *user;
  struct tcp_pcb *listener;
  all_tcp_accept_fn accept;
  all_tcp_select_fn select;
  all_tcp_recv_fn recv;
  all_tcp_poll_fn poll;
  all_tcp_close_fn close;
};

err_t all_tcp_init(struct all_tcp_handler *handler);
err_t all_tcp_free(struct all_tcp_handler *handler);
void all_tcp_close(struct all_tcp_pcb *pcb);
void all_tcp_send(struct all_tcp_pcb *pcb);
void all_tcp_send_buf(struct all_tcp_pcb *pcb, struct pbuf *buf);
void all_tcp_select(struct all_tcp_handler *handler);

#ifdef __cplusplus
}
#endif

#endif