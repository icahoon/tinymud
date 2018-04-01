#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "tinymud/mem.h"
#include "tinymud/text.h"

static const char *flushed_message = "<Output Flushed>\n";

text_block *new_text_block(const char *s, int n) {
	text_block *p;

	MALLOC(p, text_block, 1);
	MALLOC(p->buf, char, n);
	bcopy(s, p->buf, n);
	p->nchars = n;
	p->start = p->buf;
	p->next = 0;
	return p;
}

void free_text_block(text_block *t) {
	FREE(t->buf);
	FREE((char *)t);
}

void add_to_queue(text_queue *q, const char *b, int n) {
	text_block *p;

	if (n == 0) {
		return;
	}

	p = new_text_block(b, n);
	p->next = 0;
	*q->tail = p;
	q->tail = &p->next;
}

int flush_queue(text_queue *q, int n) {
	text_block *p;
	int really_flushed = 0;

	n += strlen(flushed_message);

	while (n > 0 && (p = q->head)) {
		n -= p->nchars;
		really_flushed += p->nchars;
		q->head = p->next;
		free_text_block(p);
	}
	p = new_text_block(flushed_message, strlen(flushed_message));
	p->next = q->head;
	q->head = p;
	if (!p->next) {
		q->tail = &p->next;
	}
	really_flushed -= p->nchars;
	return really_flushed;
}
