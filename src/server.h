#ifndef TINYMUD_SERVER_H
#define TINYMUD_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#include "error.h"
#include "socket.h"

extern bool server_shutdown;
extern int sock;
extern int ndescriptors;

struct server;
typedef struct server server;

struct server {
  uint16_t    port;
  uint16_t    nsockets;
  sock_t      socket;

  void (*delete)(server *s);
  error (*init)(server *s, uint16_t port);
  void (*close)(server *s);
  void (*run)(server *s);
};

extern server *new_server();

extern void close_sockets(void);

#endif /* TINYMUD_SERVER_H */
