#ifndef TINYMUD_SERVER_H
#define TINYMUD_SERVER_H

#include <stdint.h>

#include "tinymud/error.h"
#include "tinymud/socket.h"

struct server;
typedef struct server server;

struct server {
  uint16_t    port;
  sock_t      socket;

  void (*delete)(server *s);
  error (*init)(server *s, uint16_t port);
  void (*close)(server *s);
};

extern server *new_server();

#endif /* TINYMUD_SERVER_H */
