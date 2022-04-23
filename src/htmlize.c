#define _POSIX_C_SOURCE 200809L
#include "include/htmlize.h"

#include "include/escape.h"
#include "include/perror.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * _POSIX_C_SOURCE	- getline, strndup
 * ctype.h			- isdigit, isspace
 * stdbool.h		- bool, true, false
 * stdio.h			- getline, FILE
 * stdlib.h			- size_t, malloc, free, exit, EXIT_{SUCCESS,FAILURE}
 * string.h			- strcmp, strncmp
 */

#define HISTORY 2
#define READAHEAD 10
#define DATATYPE struct data *

#define BOLD_TAG	"strong"
#define ITALIC_TAG	"em"

#define free(ptr)			{ free(ptr); ptr = NULL; }
#define ifnull(ptr, val)	(ptr == NULL ? val : ptr)
#define strifnull(str)		(ifnull(str, "NULL"))

#define streql(s1, s2)		(strcmp (strifnull(s1), strifnull(s2)   ) == 0)
#define strneql(s1, s2, n)	(strncmp(strifnull(s1), strifnull(s2), n) == 0)

#define TOGGLE(boolean)	\
	boolean = boolean ? false : true;

struct config {
	bool eof;
	bool bolded;
	bool italicized;
	bool monospaced;
};
struct lines {
	char *curline;	// readahead[0]
	char *history[HISTORY];
	char *readahead[READAHEAD];
};
struct files {
	FILE *in;
	FILE *out;
};
struct link {
	char *href;
	int sublinks;
	char **sublink;
};
struct data {
	struct config *config;
	struct lines  *lines;
	struct files  *files;
	struct link   **link;
	int	   links;
};

/*
 * Utility functions.
 * Return 1 if everything is OK.
 */
static void shift_lines		(DATATYPE);
static int _get_next_line	(DATATYPE);	// Doesn't shift_lines
static int get_next_line	(DATATYPE);
static int process_curline	(DATATYPE);

/*
 * Line-wise functions.
 * Return 1 if the function changed something.
 * Should NOT get_next_line after it's done processing the current line.
 */
static int _PREFORMATTED	(DATATYPE);

/*
 * Character-wise functions.
 * Return 1 if the function changed something.
 * Should curline++ after it's done processing the current character.
 */
static int _ESCAPED_BACKSLASH	(DATATYPE);
static int _CODE				(DATATYPE);
static int _BOLD				(DATATYPE);
static int _ITALIC				(DATATYPE);

static int (*LINEWISE_FUNCTIONS[])(DATATYPE) = {
	&_PREFORMATTED,
};
static int (*CHARWISE_FUNCTIONS[])(DATATYPE) = {
	&_ESCAPED_BACKSLASH,
	&_CODE,
	&_BOLD,
	&_ITALIC,
};

#define curline  data->lines->curline
#define curchar  (curline[0])
#define prevchar (curline == data->lines->readahead[0] ? '\0' : curline[-1])
#define nextchar (curline[0] == '\0' || curline[0] == '\n' ? '\0' : curline[1])

static int
_PREFORMATTED(DATATYPE data)
{
	/*
	 * ```			=> <pre> ... </pre>
	 * ```samp		=> <pre><samp> ... </samp></pre>
	 * ```code html	=> <pre><code class="language-html"> ... </code><pre>
	 */

	if (curline != data->lines->readahead[0])
		return 0;

	if (!strneql(curline, "```", 3))
		return 0;

	curline += 3;	// For "```"
	fputs("<pre>", data->files->out);

	/*
	 * XXX: NOT `static enum`
	 * static means something different within functions
	 */
	enum {
		NONE,
		SAMP,
		CODE
	} modifier = NONE;

	if (isspace(curchar))
		;
	else if (streql(curline, "samp\n")) {
		fputs("<samp>", data->files->out);
		modifier = SAMP;
	}
	else if (strneql(curline, "code", 4)) {
		curline += 4;	// For "code"
		modifier = CODE;
		while (curchar != '\n' && isspace(curchar)) // Skip spaces
			curline++;
		if (curchar == '\n')
			fputs("<code>", data->files->out);
		else {
			char *p;
			*(p = strchr(curline, '\n')) = '\0';
			fprintf(data->files->out, "<code class=\"lanugage-%s\">", curline);
			*p = '\n';
		}
	}

	while (get_next_line(data), curline != NULL && !streql(curline, "```\n")) {
		if (streql(curline, "\\```\n"))
			curline++;
		fputs_escaped(curline, data->files->out);
	}

	switch (modifier) {
		case CODE:
			fputs("</code>", data->files->out);
			break;
		case SAMP:
			fputs("</samp>", data->files->out);
			break;
		case NONE:
			break;
	}
	fputs("</pre>\n", data->files->out);

	return 1;
}

static int
_ESCAPED_BACKSLASH(DATATYPE data)
{
	if (curchar == '\\' && nextchar == '\\') {
		fputc_escaped(curchar, data->files->out);
		curline += 2;
		return 1;
	}
	return 0;
}

static int
_CODE(DATATYPE data)
{
	if (curchar == '\\' && nextchar == '`') {
		curline++;
		fputc_escaped(curchar, data->files->out);
		curline++;
		return 1;
	}

	if (curchar != '`') {
		if (!data->config->monospaced)
			return 0;
		fputc_escaped(curchar, data->files->out);
		curline++;
		return 1;
	}

	fprintf(data->files->out, "<%scode>",  data->config->monospaced ? "/" : "");
	TOGGLE(data->config->monospaced);
	curline++;
	return 1;
}

static int
_BOLD(DATATYPE data)
{
	if (curchar == '\\' && nextchar == '*') {
		curline++;
		fputc_escaped(curchar, data->files->out);
		curline++;
		return 1;
	}

	if (curchar != '*')
		return 0;

	fprintf(data->files->out, "<%s"BOLD_TAG">",  data->config->bolded ? "/" : "");
	TOGGLE(data->config->bolded);
	curline++;
	return 1;
}

static int
_ITALIC(DATATYPE data)
{
	if (curchar == '\\' && nextchar == '_') {
		curline++;
		fputc_escaped(curchar, data->files->out);
		curline++;
		return 1;
	}

	if (curchar != '_')
		return 0;

	fprintf(data->files->out, "<%s"ITALIC_TAG">",  data->config->italicized ? "/" : "");
	TOGGLE(data->config->italicized);
	curline++;
	return 1;
}

#undef curline
#undef curchar
#undef nextchar
#undef prevchar

static void
shift_lines(DATATYPE data)
{
	/* Structure of data -
	 * 	readahead[READAHEAD-1]	<-- input
	 * 	readahead[READAHEAD-2]
	 * 	readahead[READAHEAD-3]
	 * 	...
	 * 	readahead[1]
	 * 	readahead[0]
	 * 	history[0]
	 * 	history[1]
	 * 	history[2]
	 * 	...
	 * 	history[HISTORY-2]
	 * 	history[HISTORY-1]	--> free at next iteration
	 */

	/* Shift history lines */
	free(data->lines->history[HISTORY-1]);
	for (int i = HISTORY-1; i > 0; i--) {
		if (data->lines->history[i] == NULL)
			continue;
		data->lines->history[i] = data->lines->history[i-1];
	}

	/* Shift current line to history */
	data->lines->history[0] = data->lines->readahead[0];

	/* Shift readahead lines */
	for (int i = 0; i < READAHEAD; i++) {
		data->lines->readahead[i] = data->lines->readahead[i+1];
		if (data->lines->readahead[i] == NULL)
			break;
	}
}

static int
_get_next_line(DATATYPE data)
{
	int RC = 0;

	/* If we've already reached EOF, there's nothing left to read */
	if (data->config->eof == true)
		goto end;

	/* Find an empty slot to read the new line in */
	int index = READAHEAD - 1;
	if (data->lines->readahead[index] != NULL) {
		/* No empty slot */
		RC = 1; goto end;
	}
	while (index > 0 && data->lines->readahead[index-1] == NULL)
		index--;

	/* Read next line */
	size_t _;
	if (getline(&data->lines->readahead[index], &_, data->files->in) == -1) {
		if (fgetc(data->files->in) == EOF) {
			data->config->eof = true;
			goto end;
		}
		else
			/*
			 * NOTE: It is OK to use perror here, because fgetc doesn't clobber
			 * errno. So, the errno must have been set by getline.
			 */
			exit((perror("getline error"), EXIT_FAILURE));
	}

end:
	/* Reset data->lines->curline */
	data->lines->curline = data->lines->readahead[0];
	return RC;
}

static int
get_next_line(DATATYPE data)
{
	shift_lines(data);
	return _get_next_line(data);
}

static int
process_curline(DATATYPE data)
{
	if (data->lines->curline == NULL)
		return 0;

	for (int i = 0; i < (sizeof(LINEWISE_FUNCTIONS)/sizeof(LINEWISE_FUNCTIONS[0])); i++)
		if ((LINEWISE_FUNCTIONS[i])(data))
			return 1;

	while (data->lines->curline[0] != '\0') {
		bool nobody_cares = true;
		for (int i = 0; i < (sizeof(CHARWISE_FUNCTIONS)/sizeof(CHARWISE_FUNCTIONS[0])); i++)
			if ((CHARWISE_FUNCTIONS[i])(data)) {
				/*
				 * No need to data->lines->curline++, as it has already been
				 * done by the function which cares.
				 */
				nobody_cares = false;
				break;
			}
		if (nobody_cares) {
			fputc_escaped(data->lines->curline[0], data->files->out);
			data->lines->curline++;
		}
	}

	return 1;
}

int
htmlize(FILE *src, FILE *dest)
{
	struct config config = { false };
	struct lines lines = { NULL };
	struct files files = {
		.in  = src,
		.out = dest,
	};
	struct data data = {
		.config = &config,
		.files  = &files,
		.lines  = &lines,
		.links  = 0,
	};

	for (int i = 0; i < READAHEAD; i++) {
		while (_get_next_line(&data));
	}

	while (lines.readahead[0] != NULL) {
		process_curline(&data);
		get_next_line(&data);
	}

	return EXIT_SUCCESS;
}

/* vim: set noet nowrap fdm=syntax: */
