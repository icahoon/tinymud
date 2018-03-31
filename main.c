#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/errno.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "tinymud/db.h"
#include "tinymud/externs.h"
#include "tinymud/interface.h"
#include "tinymud/config.h"

#include "tinymud/text.h"
#include "tinymud/log.h"
#include "tinymud/mem.h"
#include "tinymud/time.h"
#include "tinymud/string.h"
#include "tinymud/server.h"
#include "tinymud/connection.h"

int               shutdown_flag = 0;

static const char *connect_fail = "Either that player does not exist, or has a different password.\n";
static const char *create_fail = "Either there is already a player with that name, or that name is illegal.\n";
static const char *shutdown_message = "Going down - Bye\n";

static int sock;
int ndescriptors = 0;

void process_commands(void);
void shovechars(server *);
void do_motd(dbref);
void close_sockets();
void set_signals(void);
void parse_connect(const char *msg, char *command, char *user, char *pass);
void set_userstring(char **userstring, const char *command);
int do_command(connection *d, char *command);
void check_connect(connection *d, const char *msg);
void dump_users(connection *d, char *user);

int main(int argc, char **argv) {
	server *s;
	uint16_t port = TINYPORT;
	error err;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s infile dumpfile [port]\n", *argv);
		exit(1);
	}
	if (argc >= 4) {
		port = atoi(argv[3]);
	}

	set_signals();

	s = new_server();
	err = s->init(s, port);
	if (err != success) {
		return err;
	}

	if (init_game(argv[1], argv[2]) < 0) {
		writelog("INIT: Couldn't load %s!\n", argv[1]);
		exit(2);
	}

	/* go do it */
	shovechars(s);
	close_sockets();
	dump_database();
	return 0;
}

int notify(dbref player, const char *msg) {
	connection *d;
	int retval = 0;

	for (d = connection_list; d; d = d->next) {
		if (d->connected && d->player == player) {
			queue_string(d, msg);
			queue_write(d, "\n", 1);        /* Fuzzy: why make two packets? */
			retval = 1;
		}
	}
	return (retval);
}

struct timeval update_quotas(struct timeval last, struct timeval current) {
	int nslices;
	connection *d;

	nslices = msec_diff(current, last) / COMMAND_TIME_MSEC;

	if (nslices > 0) {
		for (d = connection_list; d; d = d->next) {
			d->quota += COMMANDS_PER_TIME * nslices;
			if (d->quota > COMMAND_BURST_SIZE) {
				d->quota = COMMAND_BURST_SIZE;
			}
		}
	}
	return msec_add(last, nslices * COMMAND_TIME_MSEC);
}

void shovechars(server *s) {
	fd_set input_set, output_set;
	long now;
	struct timeval last_slice, current_time;
	struct timeval next_slice;
	struct timeval timeout, slice_timeout;
	int maxd;
	connection *d, *dnext;
	connection *c;
	int avail_descriptors;

	sock = s->socket;
	maxd = sock+1;
	gettimeofday(&last_slice, (struct timezone *)0);

	avail_descriptors = getdtablesize() - 4;

	while (shutdown_flag == 0) {
		gettimeofday(&current_time, (struct timezone *) 0);
		last_slice = update_quotas(last_slice, current_time);

		process_commands();

		if (shutdown_flag) {
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
		for (d = connection_list; d; d=d->next) {
			if (d->input.head) {
				timeout = slice_timeout;
			} else {
				FD_SET(d->descriptor, &input_set);
			}
			if (d->output.head) {
				FD_SET(d->descriptor, &output_set);
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
			for (d = connection_list; d; d = dnext) {
				dnext = d->next;
				if (FD_ISSET(d->descriptor, &input_set)) {
					d->last_time = now;
					if (!process_input(d)) {
						shutdownsock(d);
						continue;
					}
				}
				if (FD_ISSET(d->descriptor, &output_set)) {
					if (!process_output(d)) {
						shutdownsock(d);
					}
				}
			}
		}
	}
}

void set_userstring(char **userstring, const char *command) {
	if (*userstring) {
		FREE(*userstring);
		*userstring = 0;
	}
	while (*command && isascii(*command) && isspace(*command)) {
		command++;
	}
	if (*command) {
		*userstring = strdup(command);
	}
}

void process_commands(void) {
	int nprocessed;
	connection *d, *dnext;
	struct text_block *t;

	do {
		nprocessed = 0;
		for (d = connection_list; d; d = dnext) {
			dnext = d->next;
			if (d->quota > 0 && (t = d->input.head)) {
				d->quota--;
				nprocessed++;
				if (!do_command(d, t->start)) {
					shutdownsock(d);
				} else {
					d->input.head = t->next;
					if (!d->input.head) {
						d->input.tail = &d->input.head;
					}
					free_text_block(t);
				}
			}
		}
	} while (nprocessed > 0);
}

int do_command(connection *d, char *command) {
	if (!strcmp(command, QUIT_COMMAND)) {
		goodbye_user(d);
		return 0;
	} else if (!strncmp(command, WHO_COMMAND, strlen(WHO_COMMAND))) {
		if (d->output_prefix) {
			queue_string(d, d->output_prefix);
			queue_write(d, "\n", 1);
		}
		dump_users(d, command + strlen(WHO_COMMAND));
		if (d->output_suffix) {
			queue_string(d, d->output_suffix);
			queue_write(d, "\n", 1);
		}
	} else if (d->connected &&
			   !strncmp(command, PREFIX_COMMAND, strlen(PREFIX_COMMAND))) {
#ifdef ROBOT_MODE
		if (!Robot(d->player)) {
			notify(d->player, "Only robots can use OUTPUTPREFIX; contact a Wizard.");
			return 1;
		}
		if (!d->connected) {
			return 1;
		}
#endif /* ROBOT_MODE */
		set_userstring(&d->output_prefix, command+strlen(PREFIX_COMMAND));
	} else if (d->connected &&
			   !strncmp(command, SUFFIX_COMMAND, strlen(SUFFIX_COMMAND))) {
#ifdef ROBOT_MODE
		if (!Robot(d->player)) {
			notify(d->player, "Only robots can use OUTPUTSUFFIX; contact a Wizard.");
			return 1;
		}
#endif /* ROBOT_MODE */
		set_userstring(&d->output_suffix, command+strlen(SUFFIX_COMMAND));
	} else {
		if (d->connected) {
			if (d->output_prefix) {
				queue_string(d, d->output_prefix);
				queue_write(d, "\n", 1);
			}
			process_command(d->player, command);
			if (d->output_suffix) {
				queue_string(d, d->output_suffix);
				queue_write(d, "\n", 1);
			}
		} else {
			check_connect(d, command);
		}
	}
	return 1;
}

void check_connect(connection *d, const char *msg) {
	char command[MAX_COMMAND_LEN];
	char user[MAX_COMMAND_LEN];
	char password[MAX_COMMAND_LEN];
	dbref player;

	parse_connect(msg, command, user, password);

	if (!strncmp(command, "co", 2)) {
		player = connect_player(user, password);
		if (player == NOTHING) {
			queue_string(d, connect_fail);
			writelog("FAILED CONNECT %s on %d %s\n",
					 user, d->descriptor, d->hostname);
		} else {
			writelog("CONNECTED %s(%d) on %d %s\n",
					 db[player].name, player, d->descriptor, d->hostname);
			d->connected = 1;
			d->connected_at = time(NULL);
			d->player = player;

			do_motd(player);
			do_look_around(player);
		}
	} else if (!strncmp(command, "cr", 2)) {
		player = create_player(user, password);
		if (player == NOTHING) {
			queue_string(d, create_fail);
			writelog("FAILED CREATE %s on %d %s\n",
					 user, d->descriptor, d->hostname);
		} else {
			writelog("CREATED %s(%d) on descriptor %d %s\n",
					 db[player].name, player, d->descriptor, d->hostname);
			d->connected = 1;
			d->connected_at = time(NULL);
			d->player = player;

			do_motd(player);
			do_look_around(player);
		}
	} else {
		welcome_user(d);
	}
}

void parse_connect(const char *msg, char *command, char *user, char *pass) {
	char *p;

	while (*msg && isascii(*msg) && isspace(*msg)) {
		msg++;
	}
	p = command;
	while (*msg && isascii(*msg) && !isspace(*msg)) {
		*p++ = *msg++;
	}
	*p = '\0';
	while (*msg && isascii(*msg) && isspace(*msg)) {
		msg++;
	}
	p = user;
	while (*msg && isascii(*msg) && !isspace(*msg)) {
		*p++ = *msg++;
	}
	*p = '\0';
	while (*msg && isascii(*msg) && isspace(*msg)) {
		msg++;
	}
	p = pass;
	while (*msg && isascii(*msg) && !isspace(*msg)) {
		*p++ = *msg++;
	}
	*p = '\0';
}

void close_sockets(void) {
	connection *d, *dnext;

	for (d = connection_list; d; d = dnext) {
		dnext = d->next;
		write(d->descriptor, shutdown_message, strlen(shutdown_message));
		if (shutdown(d->descriptor, 2) < 0) {
			perror("shutdown");
		}
		close(d->descriptor);
	}
	close(sock);
}

void emergency_shutdown(void) {
	close_sockets();
}

void boot_off(dbref player) {
	connection *d, *dnext;
	for (d = connection_list; d; d = dnext) {
		dnext = d->next;
		if (d->connected && d->player == player) {
			process_output(d);
			shutdownsock(d);
		}
	}
}
