#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>

#include "config.h"
#include "log.h"
#include "memutil.h"
#include "timeutil.h"
#include "stringutil.h"
#include "connection.h"
#include "interface.h"

extern int ndescriptors; /* defined in main.c */

extern void panic(const char *message);              /* defined in game.c */

struct descriptor_data *descriptor_list = 0;
char hostname[128];

void make_nonblocking(int s) {
	if (fcntl(s, F_SETFL, FNDELAY) == -1) {
		perror("make_nonblocking: fcntl");
		panic("FNDELAY fcntl failed");
	}
}

static const char *addrout(long a) {
	/* New version: returns host names, not octets.  Uses gethostbyaddr. */
	extern char *inet_ntoa(long);

	struct hostent *he;

	he = gethostbyaddr(&a, sizeof(a), AF_INET);
	if (he) {
		return he->h_name;
	}
	return inet_ntoa(a);
}

static struct descriptor_data *initializesock(int s, struct sockaddr_in *a, const char *hostname) {
	struct descriptor_data *d;

	ndescriptors++;
	MALLOC(d, struct descriptor_data, 1);
	d->descriptor = s;
	d->connected = 0;
	make_nonblocking(s);
	d->output_prefix = 0;
	d->output_suffix = 0;
	d->output_size = 0;
	d->output.head = 0;
	d->output.tail = &d->output.head;
	d->input.head = 0;
	d->input.tail = &d->input.head;
	d->raw_input = 0;
	d->raw_input_at = 0;
	d->quota = COMMAND_BURST_SIZE;
	d->last_time = 0;
	d->address = *a;                        /* added 5/3/90 SCG */
	d->hostname = alloc_string(hostname);
	if (descriptor_list) {
		descriptor_list->prev = d;
	}
	d->next = descriptor_list;
	d->prev = NULL;
	descriptor_list = d;

	welcome_user(d);
	return d;
}

static void clearstrings(struct descriptor_data *d) {
	if (d->output_prefix) {
		FREE(d->output_prefix);
		d->output_prefix = 0;
	}
	if (d->output_suffix) {
		FREE(d->output_suffix);
		d->output_suffix = 0;
	}
}

static void freeqs(struct descriptor_data *d) {
	struct text_block *cur, *next;

	cur = d->output.head;
	while (cur) {
		next = cur->next;
		free_text_block(cur);
		cur = next;
	}
	d->output.head = 0;
	d->output.tail = &d->output.head;

	cur = d->input.head;
	while (cur) {
		next = cur->next;
		free_text_block(cur);
		cur = next;
	}
	d->input.head = 0;
	d->input.tail = &d->input.head;

	if (d->raw_input) {
		FREE(d->raw_input);
	}
	d->raw_input = 0;
	d->raw_input_at = 0;
}

static void save_command(struct descriptor_data *d, const char *command) {
	add_to_queue(&d->input, command, strlen(command)+1);
}

struct descriptor_data *new_connection(int sock) {
	int newsock;
	struct sockaddr_in addr;
	unsigned int addr_len;

	addr_len = sizeof(addr);
	newsock = accept(sock, (struct sockaddr *)&addr, &addr_len);
	if (newsock < 0) {
		return 0;
	}
	strcpy(hostname, addrout(addr.sin_addr.s_addr));
	writelog("ACCEPT from %s(%d) on descriptor %d\n", hostname, ntohs(addr.sin_port), newsock);
	return initializesock(newsock, &addr, hostname);
}

void shutdownsock(struct descriptor_data *d) {
	if (d->connected) {
		writelog("DISCONNECT player %s(%d) %d %s\n",
				 db[d->player].name, d->player, d->descriptor, d->hostname);
	} else {
		writelog("DISCONNECT descriptor %d never connected\n",
				 d->descriptor);
	}
	clearstrings(d);
	shutdown(d->descriptor, 2);
	close(d->descriptor);
	freeqs(d);
	if (d->prev) {
		d->prev->next = d->next;
	} else {
		descriptor_list = d->next;
	}
	if (d->next) {
		d->next->prev = d->prev;
	}
	FREE(d);
	ndescriptors--;
}

int queue_write(struct descriptor_data *d, const char *b, int n) {
	int space;

	space = MAX_OUTPUT - d->output_size - n;
	if (space < 0) {
		d->output_size -= flush_queue(&d->output, -space);
	}
	add_to_queue(&d->output, b, n);
	d->output_size += n;
	return n;
}

int queue_string(struct descriptor_data *d, const char *s) {
	return queue_write(d, s, strlen(s));
}

int process_input(struct descriptor_data *d) {
	char buf[1024];
	int got;
	char *p, *pend, *q, *qend;

	got = read(d->descriptor, buf, sizeof buf);
	if (got <= 0) {
		return 0;
	}
	if (!d->raw_input) {
		MALLOC(d->raw_input, char, MAX_COMMAND_LEN);
		d->raw_input_at = d->raw_input;
	}
	p = d->raw_input_at;
	pend = d->raw_input + MAX_COMMAND_LEN - 1;
	for (q=buf, qend = buf + got; q < qend; q++) {
		if (*q == '\n') {
			*p = '\0';
			if (p > d->raw_input) {
				save_command(d, d->raw_input);
			}
			p = d->raw_input;
		} else if (p < pend && isascii(*q) && isprint(*q)) {
			*p++ = *q;
		}
	}
	if (p > d->raw_input) {
		d->raw_input_at = p;
	} else {
		FREE(d->raw_input);
		d->raw_input = 0;
		d->raw_input_at = 0;
	}
	return 1;
}

int process_output(struct descriptor_data *d) {
	struct text_block **qp, *cur;
	int cnt;

	for (qp = &d->output.head; (cur = *qp) != 0;) {
		cnt = write(d->descriptor, cur->start, cur->nchars);
		if (cnt < 0) {
			if (errno == EWOULDBLOCK) {
				return 1;
			}
			return 0;
		}
		d->output_size -= cnt;
		if (cnt == cur->nchars) {
			if (!cur->next) {
				d->output.tail = qp;
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

void welcome_user(struct descriptor_data *d) {
	queue_string(d, WELCOME_MESSAGE);
}

void goodbye_user(struct descriptor_data *d) {
	write(d->descriptor, LEAVE_MESSAGE, strlen(LEAVE_MESSAGE));
}

void dump_users(struct descriptor_data *e, char *user) {
	struct descriptor_data *d;
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

	d = descriptor_list;

	if (reversed)
		while (d && d->next) {
			d = d->next;
		}

	while (d) {
		if (d->connected &&
				++counter && /* Count everyone connected */
				(!user || string_prefix(db[d->player].name, user))) {
			if (god) {
#ifdef ROBOT_MODE
				sprintf(flagbuf, "%c%c%c  ",
						Flag(d->player, WIZARD) ? WIZARD_MARK : ' ',
						Flag(d->player, ROBOT)  ? ROBOT_MARK  : ' ',
						Flag(d->player, DARK)   ? DARK_MARK   : ' ');
#else
				sprintf(flagbuf, "%c%c  ",
						Flag(d->player, WIZARD) ? WIZARD_MARK : ' ',
						Flag(d->player, DARK)   ? DARK_MARK   : ' ');
#endif
			} else {
				flagbuf[0] = '\0';
			}

			if (tabular) {
				sprintf(buf, "%-16s %10s %4s",
						db[d->player].name,
						time_format_1(now - d->connected_at),
						time_format_2(now - d->last_time));
				if (wizard)
					sprintf(buf+strlen(buf),
							"  %s%s", flagbuf, d->hostname);
			} else {
				sprintf(buf,
						"%s idle %ld seconds",
						db[d->player].name,
						now - d->last_time);
				if (wizard)
					sprintf(buf+strlen(buf),
							"  %sfrom host %s", flagbuf, d->hostname);
			}
			strcat(buf, "\n");
			queue_string(e, buf);
		}
		if (reversed) {
			d = d->prev;
		} else {
			d = d->next;
		}
	}

	sprintf(buf, "%d user%s connected\n", counter, counter == 1 ? " is" : "s are");
	queue_string(e, buf);
}

int do_connect_msg(struct descriptor_data *d, const char *filename) {
	FILE           *f;
	char            buf[BUFFER_LEN];

	if ((f = fopen(filename, "r")) == NULL) {
		return (0);
	} else {
		while (fgets(buf, sizeof buf, f)) {
			queue_string(d, (char *)buf);

		}
		fclose(f);
		return (1);
	}
}
