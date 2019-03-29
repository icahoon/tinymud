#include <stdio.h>
#include <sys/time.h>
#include <time.h>

struct timeval timeval_sub(struct timeval now, struct timeval then) {
	now.tv_sec -= then.tv_sec;
	now.tv_usec -= then.tv_usec;
	if (now.tv_usec < 0) {
		now.tv_usec += 1000000;
		now.tv_sec--;
	}
	return now;
}

int msec_diff(struct timeval now, struct timeval then) {
	return ((now.tv_sec - then.tv_sec) * 1000
			+ (now.tv_usec - then.tv_usec) / 1000);
}

struct timeval msec_add(struct timeval t, int x) {
	t.tv_sec += x / 1000;
	t.tv_usec += (x % 1000) * 1000;
	if (t.tv_usec >= 1000000) {
		t.tv_sec += t.tv_usec / 1000000;
		t.tv_usec = t.tv_usec % 1000000;
	}
	return t;
}

char *time_format_1(long dt) {
	register struct tm *delta;
	static char buf[64];

	delta = gmtime(&dt);
	if (delta->tm_yday > 0)
		sprintf(buf, "%dd %02d:%02d",
				delta->tm_yday, delta->tm_hour, delta->tm_min);
	else
		sprintf(buf, "%02d:%02d",
				delta->tm_hour, delta->tm_min);
	return buf;
}

char *time_format_2(long dt) {
	register struct tm *delta;
	static char buf[64];

	delta = gmtime(&dt);
	if (delta->tm_yday > 0) {
		sprintf(buf, "%dd", delta->tm_yday);
	} else if (delta->tm_hour > 0) {
		sprintf(buf, "%dh", delta->tm_hour);
	} else if (delta->tm_min > 0) {
		sprintf(buf, "%dm", delta->tm_min);
	} else {
		sprintf(buf, "%ds", delta->tm_sec);
	}
	return buf;
}
