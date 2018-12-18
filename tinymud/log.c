#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void writelog(const char *fmt, ...) {
	va_list list;
	struct tm *tm;
	long t;
	char buffer[2048];

	va_start(list, fmt);
	vsprintf(buffer, fmt, list);
	t = time(NULL);
	tm = localtime(&t);
	fprintf(stderr, "%d/%02d %02d:%02d:%02d %s",
			tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
			buffer);
	fflush(stderr);
	va_end(list);
}
