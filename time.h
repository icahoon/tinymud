#ifndef TINYMUD_TIMEUTIL_H
#define TINYMUD_TIMEUTIL_H

extern struct timeval timeval_sub(struct timeval now, struct timeval then);
extern int msec_diff(struct timeval now, struct timeval then);
extern struct timeval msec_add(struct timeval t, int x);

extern char *time_format_1(long dt);
extern char *time_format_2(long dt);

#endif /* TINYMUD_TIMEUTIL_H */
