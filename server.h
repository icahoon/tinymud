#ifndef TINYMUD_SERVER_H
#define TINYMUD_SERVER_H

#include <stdint.h>

#include "errorutil.h"

struct server;
typedef struct server server;

typedef int sock_t;

struct server {
  uint16_t    port;
  sock_t      socket;

  void  (*delete)(server *s);
  error (*init)(server *s, uint16_t port);
  void (*close)(server *s);
};

extern server *new_server();

#endif /* TINYMUD_SERVER_H */
