#ifndef TINYMUD_MEM_H
#define TINYMUD_MEM_H

extern void panic(const char *);

#define MALLOC(result, type, number) do { \
  if (!((result) = (type *)malloc((number)*sizeof(type)))) \
    panic("Out of memory"); \
  } while (0)

#define FREE(x) (free((void *)x))

#endif /* TINYMUD_MEM_H */
