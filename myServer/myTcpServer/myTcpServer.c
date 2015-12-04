#include "mongoose.h"


static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct mbuf *io = &nc->recv_mbuf;
  (void) p;

  switch (ev) {
    case MG_EV_RECV:
      mg_send(nc, io->buf, io->len);  // Echo message back

      char *strBuf = (char*)malloc(io->len+1);
      memcpy(strBuf,io->buf,io->len);
      strBuf[io->len]=0;
      printf("received from client(sock:%d): %s",nc->sock,strBuf);
      free(strBuf);

      mbuf_remove(io, io->len);        // Discard message from recv buffer
      break;
    default:
      break;
  }
}

int main(void) {
  struct mg_mgr mgr;
  const char *port = "8800";

  mg_mgr_init(&mgr, NULL);
  mg_bind(&mgr, port, ev_handler);


  printf("Starting echo mgr on port %s\n", port);
  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
