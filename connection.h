#ifndef TINYMUD_CONNECTION_H
#define TINYMUD_CONNECTION_H

#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "tinymud/db.h"
#include "tinymud/text.h"
#include "tinymud/error.h"
#include "tinymud/socket.h"

struct connection;
typedef struct connection connection;

struct connection {
	sock_t             descriptor;
	bool               connected;
	dbref              player;
	char               *output_prefix;
	char               *output_suffix;
	int                output_size;
	struct text_queue  output;
	struct text_queue  input;
	char               *raw_input;
	char               *raw_input_at;
	long               last_time;
	long               connected_at;
	int                quota;
	struct sockaddr_in address;
	const char         *hostname;
	connection         *next;
	connection         *prev;
  
  error (*init)(connection *, sock_t);
  void (*close)(connection *);
};

extern connection *connection_list;
extern char hostname[128];

extern int queue_write(connection *c, const char *b, int n);
extern int queue_string(connection *c, const char *s);
extern int process_input(connection *c);
extern int process_output(connection *c);
extern void welcome_user(connection *c);
extern void goodbye_user(connection *c);

extern connection *new_connection();

#endif /* TINYMUD_CONNECTION_H */
