#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "tinymud/mem.h"
#include "tinymud/error.h"
#include "tinymud/connection.h"
#include "tinymud/server.h"

bool server_shutdown = false;
int sock;
int ndescriptors = 0;

static void server_delete(server *s) {
	if (s) {
		FREE(s);
	}
}

static error server_init(server *s, uint16_t port) {
	int opt;
	struct sockaddr_in addr;

	s->port = port;
	s->nsockets = 0;

	s->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("creating stream socket");
		return 3;
	}

	opt = 1;
	if (setsockopt(s->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(s->socket, (struct sockaddr *)&addr, sizeof(addr))) {
		perror("binding stream socket");
		close(s->socket);
		return 4;
	}
	if (listen(s->socket, 5) < 0) {
		perror("listen");
	}

	return success;
}

static void server_close(server *s) {
	close(s->socket);
}

server *new_server() {
	server *s;
	MALLOC(s, server, 1);
	s->delete = server_delete;
	s->init = server_init;
	s->close = server_close;
	return s;
}
/* ---- */

static const char *shutdown_message = "Going down - Bye\n";

void close_sockets(void) {
	connection *c, *cnext;

	for (c = connection_list; c; c = cnext) {
		cnext = c->next;
		write(c->descriptor, shutdown_message, strlen(shutdown_message));
		if (shutdown(c->descriptor, SHUT_RDWR) < 0) {
			perror("shutdown");
		}
		close(c->descriptor);
	}
	close(sock);
}
