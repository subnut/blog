#ifndef HTMLIZE_H
#define HTMLIZE_H

#ifndef MAX_LINKS
#define MAX_LINKS 50
#endif

#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 500
#endif

#ifndef CONTEXT_LINES
#define CONTEXT_LINES 2
#endif

int	is_named_charref(const char *);
int	htmlize(FILE *, FILE *);

enum bool {false, true};
struct config {
	enum bool BOLD_OPEN;
	enum bool ITALIC_OPEN;
	enum bool CODE_OPEN;
	enum bool CODEBLOCK_OPEN;
	enum bool HTML_TAG_OPEN;
	enum bool LINK_OPEN;
	enum bool LINK_TEXT_OPEN;
	enum bool TABLE_MODE;
	enum bool LIST_MODE;
	enum bool FOOTNOTE_MODE;
};
struct files {
	FILE *src;
	FILE *dest;
};
struct data {
	struct config *config;
	struct files  *files;
	char *line;	// Pointer to the current line
	char lines[CONTEXT_LINES*2 + 1][MAX_LINE_LENGTH];
	size_t nlines;	// Number of lines (expected to be CONTEXT_LINES*2 + 1)
};

#endif /* HTMLIZE_H */
