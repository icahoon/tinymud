#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection.h"
#include "log.h"
#include "server.h"
#include "string.h"

/* From game.c */
extern const char *dumpfile;

/* close_sockets walks through the connection_list and writes the
 * shutdown message to each connection, shuts down each connection,
 * and closes each connection.
 *
 * FIXME: This should be part of the server.
 *
 */
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

/* panic write a panic log, turns off the signals, and
 * then shut's down the server.
 *
 * FIXME: The dumping of the panic file should be part
 *        of the db code.
 */
void panic(const char *message) {
	char panicfile[2048];
	FILE *f;
	int i;

	writelog("PANIC: %s\n", message);

	/* turn off signals */
	for (i = 0; i < NSIG; i++) {
		signal(i, SIG_IGN);
	}

	/* shut down interface */
	close_sockets();

	/* dump panic file */
	sprintf(panicfile, "%s.PANIC", dumpfile);
	if ((f = fopen(panicfile, "w")) == NULL) {
		perror("CANNOT OPEN PANIC FILE, YOU LOSE:");
#ifndef NODUMPCORE
		signal(SIGILL, SIG_DFL);
		abort();
#endif /* NODUMPCORE */
		_exit(135);
	} else {
		writelog("DUMPING: %s\n", panicfile);
		db_write(f);
		fclose(f);
		writelog("DUMPING: %s (done)\n", panicfile);
#ifndef NODUMPCORE
		signal(SIGILL, SIG_DFL);
		abort();
#endif /* NODUMPCORE */
		_exit(136);
	}
}
