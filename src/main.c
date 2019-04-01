#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "db.h"
#include "externs.h"
#include "interface.h"
#include "config.h"

#include "text.h"
#include "log.h"
#include "mem.h"
#include "notify.h"
#include "time.h"
#include "string.h"
#include "signals.h"
#include "server.h"
#include "connection.h"


static const char *connect_fail = "Either that player does not exist, or has a different password.\n";
static const char *create_fail = "Either there is already a player with that name, or that name is illegal.\n";


void process_commands(void);
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
	s->run(s);
	s->close(s);
	dump_database();
	return 0;
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
