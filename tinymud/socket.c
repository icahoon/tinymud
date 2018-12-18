#include <stdio.h>
#include <fcntl.h>

#include "tinymud/socket.h"

extern void panic(const char *message); /* defined in game.c */

void set_nonblocking(int s) {
	if (fcntl(s, F_SETFL, FNDELAY) == -1) {
		perror("make_nonblocking: fcntl");
		panic("FNDELAY fcntl failed");
	}
}

