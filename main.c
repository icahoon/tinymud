#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "tinymud/signals.h"
#include "tinymud/server.h"
#include "tinymud/connection.h"


static const char *connect_fail = "Either that player does not exist, or has a different password.\n";
static const char *create_fail = "Either there is already a player with that name, or that name is illegal.\n";


void process_commands(void);
void shovechars(server *);
void do_motd(dbref);
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
	connection *c, *cnext;
	struct text_block *t;

	do {
		nprocessed = 0;
		for (c = connection_list; c; c = cnext) {
			cnext = c->next;
			if (c->quota > 0 && (t = c->input.head)) {
				c->quota--;
				nprocessed++;
				if (!do_command(c, t->start)) {
					c->close(c);
				} else {
					c->input.head = t->next;
					if (!c->input.head) {
						c->input.tail = &c->input.head;
					}
					free_text_block(t);
				}
			}
		}
	} while (nprocessed > 0);
}

int do_command(connection *c, char *command) {
	if (!strcmp(command, QUIT_COMMAND)) {
		goodbye_user(c);
		return 0;
	} else if (!strncmp(command, WHO_COMMAND, strlen(WHO_COMMAND))) {
		if (c->output_prefix) {
			queue_string(c, c->output_prefix);
			queue_write(c, "\n", 1);
		}
		dump_users(c, command + strlen(WHO_COMMAND));
		if (c->output_suffix) {
			queue_string(c, c->output_suffix);
			queue_write(c, "\n", 1);
		}
	} else if (c->connected && !strncmp(command, PREFIX_COMMAND, strlen(PREFIX_COMMAND))) {
#ifdef ROBOT_MODE
		if (!Robot(c->player)) {
			notify(c->player, "Only robots can use OUTPUTPREFIX; contact a Wizard.");
			return 1;
		}
		if (!c->connected) {
			return 1;
		}
#endif /* ROBOT_MODE */
		set_userstring(&c->output_prefix, command+strlen(PREFIX_COMMAND));
	} else if (c->connected && !strncmp(command, SUFFIX_COMMAND, strlen(SUFFIX_COMMAND))) {
#ifdef ROBOT_MODE
		if (!Robot(c->player)) {
			notify(c->player, "Only robots can use OUTPUTSUFFIX; contact a Wizard.");
			return 1;
		}
#endif /* ROBOT_MODE */
		set_userstring(&c->output_suffix, command+strlen(SUFFIX_COMMAND));
	} else {
		if (c->connected) {
			if (c->output_prefix) {
				queue_string(c, c->output_prefix);
				queue_write(c, "\n", 1);
			}
			process_command(c->player, command);
			if (c->output_suffix) {
				queue_string(c, c->output_suffix);
				queue_write(c, "\n", 1);
			}
		} else {
			check_connect(c, command);
		}
	}
	return 1;
}

void check_connect(connection *c, const char *msg) {
	char command[MAX_COMMAND_LEN];
	char user[MAX_COMMAND_LEN];
	char password[MAX_COMMAND_LEN];
	dbref player;

	parse_connect(msg, command, user, password);

	if (!strncmp(command, "co", 2)) {
		player = connect_player(user, password);
		if (player == NOTHING) {
			queue_string(c, connect_fail);
			writelog("FAILED CONNECT %s on %d %s\n", user, c->descriptor, c->hostname);
		} else {
			writelog("CONNECTED %s(%d) on %d %s\n", db[player].name, player, c->descriptor, c->hostname);
			c->connected = 1;
			c->connected_at = time(NULL);
			c->player = player;

			do_motd(player);
			do_look_around(player);
		}
	} else if (!strncmp(command, "cr", 2)) {
		player = create_player(user, password);
		if (player == NOTHING) {
			queue_string(c, create_fail);
			writelog("FAILED CREATE %s on %d %s\n", user, c->descriptor, c->hostname);
		} else {
			writelog("CREATED %s(%d) on descriptor %d %s\n", db[player].name, player, c->descriptor, c->hostname);
			c->connected = 1;
			c->connected_at = time(NULL);
			c->player = player;

			do_motd(player);
			do_look_around(player);
		}
	} else {
		welcome_user(c);
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

void emergency_shutdown(void) {
	close_sockets();
}

void boot_off(dbref player) {
	connection *c, *cnext;
	for (c = connection_list; c; c = cnext) {
		cnext = c->next;
		if (c->connected && c->player == player) {
			process_output(c);
			c->close(c);
		}
	}
}

/* this is awful */
int notify(dbref player, const char *msg) {
	connection *c;
	int retval = 0;

	for (c = connection_list; c; c = c->next) {
		if (c->connected && c->player == player) {
			queue_string(c, msg);
			queue_write(c, "\n", 1);        /* Fuzzy: why make two packets? */
			retval = 1;
		}
	}
	return (retval);
}
