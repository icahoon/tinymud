/* #include <stdio.h> */
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>

#include "tinymud/config.h"
#include "tinymud/interface.h"

#include "tinymud/log.h"
#include "tinymud/mem.h"
#include "tinymud/time.h"
#include "tinymud/string.h"
#include "tinymud/connection.h"

extern int ndescriptors; /* defined in main.c */

connection *connection_list = 0;
char hostname[128];

static const char *addr_out(long a) {
	/* New version: returns host names, not octets.  Uses gethostbyaddr. */
	extern char *inet_ntoa(long);

	struct hostent *he;

	he = gethostbyaddr(&a, sizeof(a), AF_INET);
	if (he) {
		return he->h_name;
	}
	return inet_ntoa(a);
}

static void clearstrings(connection *c) {
	if (c->output_prefix) {
		FREE(c->output_prefix);
		c->output_prefix = 0;
	}
	if (c->output_suffix) {
		FREE(c->output_suffix);
		c->output_suffix = 0;
	}
}

static void freeqs(connection *c) {
	struct text_block *cur, *next;

	cur = c->output.head;
	while (cur) {
		next = cur->next;
		free_text_block(cur);
		cur = next;
	}
	c->output.head = 0;
	c->output.tail = &c->output.head;

	cur = c->input.head;
	while (cur) {
		next = cur->next;
		free_text_block(cur);
		cur = next;
	}
	c->input.head = 0;
	c->input.tail = &c->input.head;

	if (c->raw_input) {
		FREE(c->raw_input);
	}
	c->raw_input = 0;
	c->raw_input_at = 0;
}

static void save_command(connection *c, const char *command) {
	add_to_queue(&c->input, command, strlen(command)+1);
}

int queue_write(connection *c, const char *b, int n) {
	int space;

	space = MAX_OUTPUT - c->output_size - n;
	if (space < 0) {
		c->output_size -= flush_queue(&c->output, -space);
	}
	add_to_queue(&c->output, b, n);
	c->output_size += n;
	return n;
}

int queue_string(connection *c, const char *s) {
	return queue_write(c, s, strlen(s));
}

int process_input(connection *c) {
	char buf[1024];
	int got;
	char *p, *pend, *q, *qend;

	got = read(c->descriptor, buf, sizeof buf);
	if (got <= 0) {
		return 0;
	}
	if (!c->raw_input) {
		MALLOC(c->raw_input, char, MAX_COMMAND_LEN);
		c->raw_input_at = c->raw_input;
	}
	p = c->raw_input_at;
	pend = c->raw_input + MAX_COMMAND_LEN - 1;
	for (q=buf, qend = buf + got; q < qend; q++) {
		if (*q == '\n') {
			*p = '\0';
			if (p > c->raw_input) {
				save_command(c, c->raw_input);
			}
			p = c->raw_input;
		} else if (p < pend && isascii(*q) && isprint(*q)) {
			*p++ = *q;
		}
	}
	if (p > c->raw_input) {
		c->raw_input_at = p;
	} else {
		FREE(c->raw_input);
		c->raw_input = 0;
		c->raw_input_at = 0;
	}
	return 1;
}

int process_output(connection *c) {
	struct text_block **qp, *cur;
	int cnt;

	for (qp = &c->output.head; (cur = *qp) != 0;) {
		cnt = write(c->descriptor, cur->start, cur->nchars);
		if (cnt < 0) {
			if (errno == EWOULDBLOCK) {
				return 1;
			}
			return 0;
		}
		c->output_size -= cnt;
		if (cnt == cur->nchars) {
			if (!cur->next) {
				c->output.tail = qp;
			}
			*qp = cur->next;
			free_text_block(cur);
			continue;                /* do not adv ptr */
		}
		cur->nchars -= cnt;
		cur->start += cnt;
		break;
	}
	return 1;
}

void welcome_user(connection *c) {
	queue_string(c, WELCOME_MESSAGE);
}

void goodbye_user(connection *c) {
	write(c->descriptor, LEAVE_MESSAGE, strlen(LEAVE_MESSAGE));
}

void dump_users(connection *e, char *user) {
	connection *c;
	long now;
	char buf[1024], flagbuf[16];
	int wizard, god;
	int counter=0;
	int reversed, tabular;

	while (*user && isspace(*user)) {
		user++;
	}
	if (!*user) {
		user = NULL;
	}

	reversed = e->connected && Flag(e->player, REVERSED_WHO);
	tabular = e->connected && Flag(e->player, TABULAR_WHO);

	(void)time(&now);
	queue_string(e, tabular ? "Player Name          On For Idle\n" : "Current Players:\n");

#ifdef GOD_MODE
	god = wizard = e->connected && God(e->player);
#else  /* GOD_MODE */
	god = e->connected && God(e->player);
	wizard = e->connected && Wizard(e->player);
#endif /* GOD_MODE */

	c = connection_list;

	if (reversed)
		while (c && c->next) {
			c = c->next;
		}

	while (c) {
		if (c->connected &&
				++counter && /* Count everyone connected */
				(!user || string_prefix(db[c->player].name, user))) {
			if (god) {
#ifdef ROBOT_MODE
				sprintf(flagbuf, "%c%c%c  ",
						Flag(c->player, WIZARD) ? WIZARD_MARK : ' ',
						Flag(c->player, ROBOT)  ? ROBOT_MARK  : ' ',
						Flag(c->player, DARK)   ? DARK_MARK   : ' ');
#else
				sprintf(flagbuf, "%c%c  ",
						Flag(c->player, WIZARD) ? WIZARD_MARK : ' ',
						Flag(c->player, DARK)   ? DARK_MARK   : ' ');
#endif
			} else {
				flagbuf[0] = '\0';
			}

			if (tabular) {
				sprintf(buf, "%-16s %10s %4s",
						db[c->player].name,
						time_format_1(now - c->connected_at),
						time_format_2(now - c->last_time));
				if (wizard)
					sprintf(buf+strlen(buf),
							"  %s%s", flagbuf, c->hostname);
			} else {
				sprintf(buf,
						"%s idle %ld seconds",
						db[c->player].name,
						now - c->last_time);
				if (wizard)
					sprintf(buf+strlen(buf),
							"  %sfrom host %s", flagbuf, c->hostname);
			}
			strcat(buf, "\n");
			queue_string(e, buf);
		}
		if (reversed) {
			c = c->prev;
		} else {
			c = c->next;
		}
	}

	sprintf(buf, "%d user%s connected\n", counter, counter == 1 ? " is" : "s are");
	queue_string(e, buf);
}

int do_connect_msg(connection *c, const char *filename) {
	FILE           *f;
	char            buf[BUFFER_LEN];

	if ((f = fopen(filename, "r")) == NULL) {
		return (0);
	} else {
		while (fgets(buf, sizeof buf, f)) {
			queue_string(c, (char *)buf);

		}
		fclose(f);
		return (1);
	}
}

static error connection_init(connection *c, sock_t s) {
	int newsock;
	struct sockaddr_in addr;
	unsigned int addr_len;

	addr_len = sizeof(addr);
	newsock = accept(s, (struct sockaddr *)&addr, &addr_len);
	if (newsock < 0) {
		return !success;
	}

	strcpy(hostname, addr_out(addr.sin_addr.s_addr));
	writelog("ACCEPT from %s(%d) on descriptor %d\n", hostname, ntohs(addr.sin_port), newsock);

	ndescriptors++;
	c->descriptor = newsock;
	set_nonblocking(newsock);
	c->connected = false;
	c->output_prefix = 0;
	c->output_suffix = 0;
	c->output_size = 0;
	c->output.head = 0;
	c->output.tail = &c->output.head;
	c->input.head = 0;
	c->input.tail = &c->input.head;
	c->raw_input = 0;
	c->raw_input_at = 0;
	c->quota = COMMAND_BURST_SIZE;
	c->last_time = 0;
	c->address = addr;
	c->hostname = alloc_string(hostname);
	if (connection_list) {
		connection_list->prev = c;
	}
	c->next = connection_list;
	c->prev = NULL;
	connection_list = c;

	welcome_user(c);
	return success;
}

static void connection_close(connection *c) {
	if (c->connected) {
		writelog("DISCONNECT player %s(%d) %d %s\n",
				 db[c->player].name, c->player, c->descriptor, c->hostname);
	} else {
		writelog("DISCONNECT descriptor %d never connected\n",
				 c->descriptor);
	}
	clearstrings(c);
	shutdown(c->descriptor, SHUT_RDWR);
	close(c->descriptor);
	freeqs(c);
	if (c->prev) {
		c->prev->next = c->next;
	} else {
		connection_list = c->next;
	}
	if (c->next) {
		c->next->prev = c->prev;
	}
	FREE(c);
	ndescriptors--;
}

connection *new_connection() {
	connection *c;
	MALLOC(c, connection, 1);
	c->init = connection_init;
	c->close = connection_close;
	return c;
}
