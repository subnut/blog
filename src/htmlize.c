#define _POSIX_C_SOURCE 200809L
#include "include/proto/htmlize.h"

#include "include/proto/escape.h"
#include "include/proto/charref.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * _POSIX_C_SOURCE	- getline, strndup
 * ctype.h			- isdigit, isspace
 * stdbool.h		- bool, true, false
 * stdio.h			- getline, FILE
 * stddef.h			- NULL
 * stdlib.h			- size_t, malloc, exit, EXIT_{SUCCESS,FAILURE}
 * string.h			- strcmp, strncmp
 */

#include "include/defs/free.h"
#include "include/defs/ifnull.h"
#include "include/defs/perror.h"
#include "include/defs/streql.h"

#define HISTORY 2
#define READAHEAD 10
#define DATATYPE struct data *

#define BOLD_TAG	"strong"
#define ITALIC_TAG	"em"

#define TOGGLE(boolean)	\
	boolean = boolean ? false : true;

struct config {
	bool eof;
	bool bolded;
	bool italicized;
	bool monospaced;
	int	 h_level;
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
 *
 * RATIONALE: This whole line is mine. I'm gonna do whatever I want with it,
 * and then peacefully retire without having to worry about somebody else
 * trying to read this line.
 */
static int _PREFORMATTED	(DATATYPE);

/*
 * Character-wise functions.
 * Return 1 if the function changed something.
 * Should curline++ after it's done processing the current character.
 *
 * RATIONALE: I'm gonna consume all the characters that are important to me, so
 * that nobody else tries to use those characters for themselves.
 */
static int _ESCAPED_BACKSLASH	(DATATYPE);
static int _HEADINGS			(DATATYPE);
static int _CHARREF				(DATATYPE);
static int _CODE				(DATATYPE);
static int _BOLD				(DATATYPE);
static int _ITALIC				(DATATYPE);

static int (*LINEWISE_FUNCTIONS[])(DATATYPE) = {
	&_PREFORMATTED,
};
static int (*CHARWISE_FUNCTIONS[])(DATATYPE) = {
	&_ESCAPED_BACKSLASH,
	&_HEADINGS,
	&_CHARREF,
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
_HEADINGS(DATATYPE data)
{
	if (data->config->h_level != 0 && curchar == '\n') {
		fprintf(data->files->out, "</h%i>\n", data->config->h_level);
		data->config->h_level = 0;
		curline++;
		return 1;
	}

	if (curline != data->lines->readahead[0])
		return 0;

	if (curchar == '\\' && nextchar == '#') {
		for (; curchar == '#'; curline++)
			fputc_escaped(curchar, data->files->out);
		return 1;
	}

	if (curchar != '#')
		return 0;

	int level = 1;	// "" => <h1>, "#" => <h2>, "##" => <h3>, ...
	while (*curline++ == '#') level++;
	while (*curline	  == ' ') curline++;
	fprintf(data->files->out, "<h%i>", level);
	data->config->h_level = level;

	curline++;
	return 1;
}

static int
_CHARREF(DATATYPE data)
{
	if (!is_charref(curline))
		return 0;

	char storage;
	char *end = strchr(curline, ';') + 1;
	storage = end[0];
	end[0] = '\0';
	fputs(curline, data->files->out);
	end[0] = storage;
	curline = end;
	return 1;
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
		goto failure;

	/* Find an empty slot to read the new line in */
	int index = READAHEAD - 1;
	if (data->lines->readahead[index] != NULL)
		goto failure; /* No empty slot */
	while (index > 0 && data->lines->readahead[index-1] == NULL)
		index--;

	/* Read next line */
	size_t _;
	if (getline(&data->lines->readahead[index], &_, data->files->in) == -1) {
		if (fgetc(data->files->in) == EOF) {
			data->config->eof = true;
			goto success;
		}
		else
			/*
			 * NOTE: It is OK to use perror here, because fgetc doesn't clobber
			 * errno. So, the errno must have been set by getline.
			 */
			exit((perror("getline error"), EXIT_FAILURE));
	}

success:
	RC = 1;

failure:
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
	struct config config = { 0 };
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
