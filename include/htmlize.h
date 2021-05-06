#ifndef HTMLIZE_H
#define HTMLIZE_H

#ifndef MAX_LINKS
#define MAX_LINKS 50
#endif

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 500
#endif

void	fputc_escaped(char, FILE *);
void	fputs_escaped(const char *, FILE *);
int	is_named_charref(const char *);
void	htmlize(FILE *, FILE *);

#endif /* HTMLIZE_H */
