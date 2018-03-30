#ifndef TINYMUD_TEXT_H
#define TINYMUD_TEXT_H


typedef struct text_block {
  int               nchars;
  struct text_block *next;
  char              *start;
  char              *buf;
} text_block;

extern text_block *new_text_block(const char *s, int n);
extern void free_text_block(text_block *t);

typedef struct text_queue {
  struct text_block *head;
  struct text_block **tail;
} text_queue;

extern void add_to_queue(text_queue *q, const char *b, int n);
extern int flush_queue(text_queue *q, int n);

#endif /* TINYMUD_TEXT_H */
