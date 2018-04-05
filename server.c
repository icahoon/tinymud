#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>

#include "tinymud/config.h"
#include "tinymud/mem.h"
#include "tinymud/error.h"
#include "tinymud/time.h"
#include "tinymud/connection.h"
#include "tinymud/server.h"

extern void process_commands(void); /* main.c */

static const char *shutdown_message = "Going down - Bye\n";

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
	connection *c, *cnext;

	for (c = connection_list; c; c = cnext) {
		cnext = c->next;
		write(c->descriptor, shutdown_message, strlen(shutdown_message));
		if (shutdown(c->descriptor, SHUT_RDWR) < 0) {
			perror("shutdown");
		}
		close(c->descriptor);
	}
	close(s->socket);
}

static struct timeval update_quotas(struct timeval last, struct timeval current) {
	int nslices;
	connection *c;

	nslices = msec_diff(current, last) / COMMAND_TIME_MSEC;

	if (nslices > 0) {
		for (c = connection_list; c; c = c->next) {
			c->quota += COMMANDS_PER_TIME * nslices;
			if (c->quota > COMMAND_BURST_SIZE) {
				c->quota = COMMAND_BURST_SIZE;
			}
		}
	}
	return msec_add(last, nslices * COMMAND_TIME_MSEC);
}

static void server_run(server *s) {
	fd_set input_set, output_set;
	long now;
	struct timeval last_slice, current_time;
	struct timeval next_slice;
	struct timeval timeout, slice_timeout;
	int maxd;
	connection *c, *cnext;
	int avail_descriptors;

	sock = s->socket;
	maxd = sock+1;
	gettimeofday(&last_slice, (struct timezone *)0);

	avail_descriptors = getdtablesize() - 4;

	while (!server_shutdown) {
		gettimeofday(&current_time, (struct timezone *) 0);
		last_slice = update_quotas(last_slice, current_time);

		process_commands();

		if (server_shutdown) {
			break;
		}
		timeout.tv_sec = 1000;
		timeout.tv_usec = 0;
		next_slice = msec_add(last_slice, COMMAND_TIME_MSEC);
		slice_timeout = timeval_sub(next_slice, current_time);

		FD_ZERO(&input_set);
		FD_ZERO(&output_set);
		if (ndescriptors < avail_descriptors) {
			FD_SET(sock, &input_set);
		}
		for (c = connection_list; c; c = c->next) {
			if (c->input.head) {
				timeout = slice_timeout;
			} else {
				FD_SET(c->descriptor, &input_set);
			}
			if (c->output.head) {
				FD_SET(c->descriptor, &output_set);
			}
		}

		if (select(maxd, &input_set, &output_set,
				   (fd_set *) 0, &timeout) < 0) {
			if (errno != EINTR) {
				perror("select");
				return;
			}
		} else {
			error err;

			time(&now);
			if (FD_ISSET(sock, &input_set)) {
				c = new_connection();
				err = c->init(c, sock);
				if (err != success) {
					if (errno && errno != EINTR && errno != EMFILE && errno != ENFILE) {
						perror("new_connection");
						return;
					}
				} else {
					if (c->descriptor >= maxd) {
						maxd = c->descriptor + 1;
					}
				}
			}
			for (c = connection_list; c; c = cnext) {
				cnext = c->next;
				if (FD_ISSET(c->descriptor, &input_set)) {
					c->last_time = now;
					if (!process_input(c)) {
						c->close(c);
						continue;
					}
				}
				if (FD_ISSET(c->descriptor, &output_set)) {
					if (!process_output(c)) {
						c->close(c);
					}
				}
			}
		}
	}
}

server *new_server() {
	server *s;
	MALLOC(s, server, 1);
	s->delete = server_delete;
	s->init = server_init;
	s->close = server_close;
	s->run = server_run;
	return s;
}
/* ---- */

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
