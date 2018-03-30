#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

extern void panic(const char *message);   /* defined in game.c */

void make_nonblocking(int s) {
	if (fcntl(s, F_SETFL, FNDELAY) == -1) {
		perror("make_nonblocking: fcntl");
		panic("FNDELAY fcntl failed");
	}
}
