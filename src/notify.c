#include "connection.h"
#include "db.h"

/* notify walks the the connection_list (global connection.c) and if it finds
 * a connection that matches with the given player, is enqueue's the given
 * message to that connection.
 */
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
