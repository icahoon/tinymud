#ifndef TINYMUD_STRINGUTIL_H
#define TINYMUD_STRINGUTIL_H

extern int string_compare(const char *s1, const char *s2);
extern int string_prefix(const char *string, const char *prefix);
extern const char *string_match(const char *src, const char *sub);
extern char *strsave(const char *s);

#endif /* TINYMUD_STRINGUTIL_H */
