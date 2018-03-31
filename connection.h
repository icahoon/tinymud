#ifndef TINYMUD_CONNECTION_H
#define TINYMUD_CONNECTION_H

#include <sys/socket.h>
#include <netinet/in.h>

#include "db.h"
#include "text.h"

struct descriptor_data {
	int descriptor;
	int connected;
	dbref player;
	char *output_prefix;
	char *output_suffix;
	int output_size;
	struct text_queue output;
	struct text_queue input;
	char *raw_input;
	char *raw_input_at;
	long last_time;
	long connected_at;
	int quota;
	struct sockaddr_in address;
	const char *hostname;
	struct descriptor_data *next;
	struct descriptor_data *prev;
};

extern struct descriptor_data * descriptor_list;
extern char hostname[128];

extern struct descriptor_data *new_connection(int sock);
extern void shutdownsock(struct descriptor_data *d);
extern int queue_write(struct descriptor_data *d, const char *b, int n);
extern int queue_string(struct descriptor_data *d, const char *s);
extern int process_input(struct descriptor_data *d);
extern int process_output(struct descriptor_data *d);
extern void welcome_user(struct descriptor_data *d);
extern void goodbye_user(struct descriptor_data *d);

#endif /* TINYMUD_CONNECTION_H */
