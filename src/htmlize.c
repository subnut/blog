#define _POSIX_C_SOURCE 200809L
#include "include/htmlize.h"

#include "constants.h"
#include "include/escape.h"

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
 * stdlib.h			- free, strtol, exit, EXIT_SUCCESS, EXIT_FAILURE
 * string.h			- strncmp, strchr, strrchr
 */

#define perror(str) \
	(fprintf(stderr,"%d:%s:%s(): ",__LINE__,__FILE__,__func__), perror(str))

#define streql(s1, s2) (strcmp(s1, s2) == 0)
#define strneql(s1, s2, n) (strncmp(s1, s2, n) == 0)

#define LINK_DEFINED		11
#define LINK_UNDEFINED		12
static int local_errno;

/* Struct definitions */
struct config {
	bool bolded;
	bool italicized;
};
struct lines {
	char *curline;	// Should be set at readahead[0] before every pass
	char *history	[HISTORY_LINES];
	char *readahead	[READAHEAD_LINES];
};
struct files {
	FILE *in;
	FILE *out;
};
struct data {
	struct config *config;
	struct lines  *lines;
	struct files  *files;
	struct link   *links;
};
struct link {
	int maxindex;
	char *link;
	struct link **links;
};

/* Function prototypes */
static int  valid_linkid			(const char *);
static char *get_linkdef			(struct link *, const char *);
static char *get_linkdef_			(struct link *, const char *);
static void free_linkdef			(struct link *, const char *);
static void free_linkdef_			(struct link *, const char *);
static void set_linkdef				(struct link *, const char *, char *);
static int  set_linkdef_			(struct link *, const char *, char *);
static void free_links_recursively	(struct link *);
static void get_next_line			(struct data *);
static int _LINKDEF					(struct data *);
static int _PREFORMATTED			(struct data *);
static int _CODE					(struct data *);

static int (*LINEWISE_FUNCTIONS[])(struct data *) = {
	&_PREFORMATTED,
	&_LINKDEF,
};
static int (*CHARWISE_FUNCTIONS[])(struct data *) = {
	&_CODE,
};

static int
valid_linkid(const char *id)
#define failprintf(...) { fprintf(__VA_ARGS__); return 0; }
{
	char *end;
	end = strchr(id, ']');

	/* If no terminating ']', it's not an ID */
	if (end == NULL)
		failprintf(stderr, "Not a link id: %s\n", id);


	/* ID can consist of digits and '.' only */
	for (const char *c = id; c < end; c++)
		if (*c != '.' && !isdigit(*c))	// Only numbers and '.' allowed
			failprintf(stderr,
					"Invalid link id: [%s]\n"
					"Only digits and dot (.) allowed\n",
					strndup(id, end-id));


	/* Check all sub-indexes */
	const char *ptr;
	ptr = id;

	int index;
	index = (int)strtol(ptr, &end, 10);
	while (*end != ']' && index > 0 && end != ptr)
	{
		ptr = end + 1;
		index = (int)strtol(ptr, &end, 10);
	}

	if (end == ptr)	// [X..Y] or [.X.Y] or [X.Y.]
		failprintf(stderr,
				"Invalid link id: [%s]\n"
				"Please ensure all indexes are specified\n",
				strndup(id, strchr(id,']')-id));

	if (index < 0)
		failprintf(stderr,
				"Invalid link id: [%s]\n"
				"Please ensure all indexes are positive\n",
				strndup(id, strchr(id,']')-id));

	if (index == 0) // [X.0.Y] or [0] or [0.Y]
		failprintf(stderr,
				"Invalid link id: [%s]\n"
				"Please ensure all indexes are non-zero\n",
				strndup(id, strchr(id,']')-id));

	/* ID is valid */
	return 1;
}
#undef failprintf

static char *
get_linkdef(struct link *links, const char *id)
{
	if (!valid_linkid(id))
		exit(EXIT_FAILURE);

	char *retval;
	retval = get_linkdef_(links, id);
	if (retval != NULL)
		return retval;

	switch (local_errno)
	{
		case LINK_UNDEFINED:
			fprintf(stderr,
					"Link not defined: [%s]\n"
					"Please define the link before using it\n",
					strndup(id, strchr(id,']')-id));
			break;
	}
	exit(EXIT_FAILURE);
}

static char *
get_linkdef_(struct link *links, const char *id)
{
	int index;
	char *end;
	index = (int)strtol(id, &end, 10);

	if (links->maxindex < index)
		return (local_errno = LINK_UNDEFINED),
			   NULL;

	if (*end == ']')
		return links->links[index]->link;
	else /* (*end == '.') */
		return get_linkdef_(links->links[index], end + 1);
}

static void
free_linkdef(struct link *links, const char *id)
{
	if (!valid_linkid(id)) exit(EXIT_FAILURE);
	return free_linkdef_(links, id);
}

static void
free_linkdef_(struct link *links, const char *id)
{
	// TODO. Free linkdef. And free its parent structure if it becomes empty.
}

static void
set_linkdef(struct link *links, const char *id, char *href)
{
	if (!valid_linkid(id))
		exit(EXIT_FAILURE);

	switch (set_linkdef_(links, id, href))
	{
		case EXIT_SUCCESS:
			return;

		case LINK_DEFINED:
			fprintf(stderr,
					"Link already defined: [%s]\n"
					"Please use it before defining it again.\n",
					strndup(id, strchr(id,']')-id));
			exit(EXIT_FAILURE);
	}
}

static int
set_linkdef_(struct link *links, const char *id, char *href)
{
	int index;
	char *end;
	index = (int)strtol(id, &end, 10);

	if (links->maxindex == 0)
	{
		/* Allocate */
		if ((links->links = calloc(index + 1, sizeof(struct link *))) == NULL)
			exit((perror("calloc failed"), EXIT_FAILURE));

		/* Initialize */
		links->link = NULL;
		links->maxindex = index;
		for (int i = 0; i <= index; i++)
			links->links[i] = NULL;
	}
	else if (links->maxindex < index)
	{
		/* Allocate new indices */
		if ((links->links = realloc(links->links, (index + 1) * sizeof(struct link *))) == NULL)
			exit((perror("realloc failed"), EXIT_FAILURE));

		/* Initialize new indices */
		for (int i = links->maxindex + 1; i <= index; i++)
			links->links[i] = NULL;
		links->maxindex = index;
	}

	if (*end == ']')
	{
		if (links->links[index] != NULL)
			if (links->links[index]->link != NULL)
				return LINK_DEFINED;

		/* Allocate if required */
		if (links->links[index] == NULL)
			if ((links->links[index] = malloc(sizeof(struct link))) == NULL)
				exit((perror("malloc failed"), EXIT_FAILURE));

		/* Initialize */
		links->links[index]->maxindex = 0;
		links->links[index]->links = NULL;
		links->links[index]->link = href;
	}
	else /* (*end == '.') */
	{
		/* Allocate if required */
		if (links->links[index] == NULL)
			if ((links->links[index] = malloc(sizeof(struct link))) == NULL)
				exit((perror("malloc failed"), EXIT_FAILURE));

		return set_linkdef_(links->links[index], end + 1, href);
	}

	return EXIT_SUCCESS;
}

static void
free_links_recursively(struct link *links)
#define free(ptr) { free(ptr); ptr = NULL; }
{
	if (links->link != NULL)
		free(links->link);

	if (links->maxindex == 0)
		return;

	for (int index = 0; index <= links->maxindex; index++)
	{
		if (links->links[index] == NULL) continue;
		else
		{
			free_links_recursively(links->links[index]);
			free(links->links[index]);
		}
	}
}
#undef free

static void
get_next_line(struct data *data)
{
	/* Structure of data -
	 * 	readahead[READAHEAD_LINES-1]	<-- input
	 * 	readahead[READAHEAD_LINES-2]
	 * 	readahead[READAHEAD_LINES-3]
	 * 	...
	 * 	readahead[1]
	 * 	readahead[0]
	 * 	history[0]
	 * 	history[1]
	 * 	history[2]
	 * 	...
	 * 	history[HISTORY_LINES-2]
	 * 	history[HISTORY_LINES-1]	--> free at next iteration
	 */

	/* Shift lines */
	free(data->lines->history[HISTORY_LINES-1]);
	for (int i = HISTORY_LINES-1; i>0; i--)
		data->lines->history[i] = data->lines->history[i-1];

	data->lines->history[0] = data->lines->readahead[0];
	for (int i=1; i < READAHEAD_LINES; i++)
		data->lines->readahead[i-1] = data->lines->readahead[i];

	/*
	 * Avoid the situation where we read more than we need.
	 *
	 * If any of the previous lines is NULL (ie. we had reached EOF or "---\n")
	 * then set new line to NULL and return.
	 */
	for (int i = READAHEAD_LINES-1; i>0; i--)
		if (data->lines->readahead[i] == NULL)
		{
			data->lines->readahead[READAHEAD_LINES-1] = NULL;
			goto success;
		}

	/* Read next line */
	static size_t _ = 0;
	data->lines->readahead[READAHEAD_LINES-1] = NULL;
	if (getline(&data->lines->readahead[READAHEAD_LINES-1], &_, data->files->in) == -1)
	{
		if (fgetc(data->files->in) == EOF)
			/*
			 * We have reached EOF. Set new line to NULL to indicate this fact
			 * to future invocations
			 */
			data->lines->readahead[READAHEAD_LINES-1] = NULL;
		else
			/*
			 * We haven't reached EOF. So, getline errored out due to a
			 * different (possibly dangerous) reason. Use perror to inform the
			 * user about this error, and exit(EXIT_FAILURE)
			 *
			 * NOTE: It is OK to use perror here, because fgetc doesn't clobber
			 * errno. So, the errno must have been set by getline.
			 */
			exit((perror("getline error"), EXIT_FAILURE));
	}

	/* If we have read "---\n", set new line to NULL to indicate that */
	if (strneql(data->lines->readahead[READAHEAD_LINES-1], "---\n", 4))
	{
		free(data->lines->readahead[READAHEAD_LINES-1]);
		data->lines->readahead[READAHEAD_LINES-1] = NULL;
		goto success;
	}

success:
	/* Reset lines.curline to lines.readahead[0] */
	data->lines->curline = data->lines->readahead[0];
}

/*
 * Functions shall be of the form -
 * 	static int _name(struct data *)
 *
 * Functions shall return 0 if it did nothing, and 1 if it did something.
 * So,
 * 	for (i=0;something;i++)
 * 		if ((functions[i])(&data))
 * 			continue;
 */

static int
_LINKDEF(struct data *data)
#define curline data->lines->curline
{
	/*
	 * Linkdefs shall be of the form:
	 *
	 * |
	 * |	[1]:   some-link
	 * |	[2.1]: some-other-link
	 * |	[2.2]:    mis-aligned-link
	 * |	[2.10]:hey-im-a-link
	 * |
	 *
	 * A link defined once can be used only once. After that, it is undefined.
	 * A link already defined cannot be redefined without using it first.
	 */

	char *id;
	char *end;
	char *href;

	if (curline[0] != '\t')						return 0;
	if (curline[1] != '[')						return 0;
	if ((href = strchr(curline, ':')) == NULL)	return 0;

	while (isspace(href[0])) href++;	// ltrim
	if (href[0] == '\0') return 0;	// EOL reached! ie. No link given, only id!
	end = strrchr(curline, '\0');
	while (end > href && isspace(end[-1])) end--;	// rtrim
	if ((href = strndup(href, end - href)) == NULL)
		exit((perror("strndup failed"), EXIT_FAILURE));

	id = curline + 2;	// \t[
	if (!valid_linkid(id))
		exit(EXIT_FAILURE);

	set_linkdef(data->links, id, href);
	return 1;
}
#undef curline

static int
_PREFORMATTED(struct data *data)
#define curline data->lines->curline
{
	/*
	 * ```			=> <pre> ... </pre>
	 * ```samp		=> <pre><samp> ... </code><samp>
	 * ```code html	=> <pre><code class="language-html"> ... </code><pre>
	 */

	if (!strneql(curline, "```", 3))
		return 0;

	curline += 3;	// For "```"
	fputs("<pre>", data->files->out);

	static enum {
		NONE,
		SAMP,
		CODE
	} modifier = NONE;

	if (!isspace(*curline))
	{
		if (streql(curline, "samp\n"))
		{
			fputs("<samp>", data->files->out);
			modifier = SAMP;
		}
		else if (strneql(curline, "code", 4))
		{
			curline += 4;	// For "code"
			modifier = CODE;
			if (*curline != '\n')
			{
				/* Skip over all whitespaces */
				while (isspace(*curline))
					curline++;

				char *p;
				*(p = strchr(curline, '\n')) = '\0';
				fprintf(data->files->out, "<code class=\"lanugage-%s\">", curline);
				*p = '\n';
			}
			else
				fputs("<code>", data->files->out);
		}
	}

	while (get_next_line(data), curline != NULL && !streql(curline, "```\n"))
		fputs_escaped(curline, data->files->out);

	switch (modifier)
	{
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

	if (streql(curline, "```\n"))
		get_next_line(data);

	return 1;
}
#undef curline

static int
_CODE(struct data *data)
#define curline data->lines->curline
{
	if (*curline != '`')
		return 0;

	fputs("<code>", data->files->out);
	for (curline++; *curline != '`'; curline++)
	{
		if (*curline == '\0') get_next_line(data);
		if (curline == NULL) break;
		fputc_escaped(*curline, data->files->out);
	}
	fputs("</code>", data->files->out);
	if (*curline == '`') curline++;

	return 1;
}
#undef curline

int
htmlize(FILE *src, FILE *dest)
{
	/* Initialize structs */
	struct config config;
	struct lines lines;
	struct files files;
	struct link links;
	struct data data;

	/* Populate links */
	links.link = NULL;
	links.maxindex = 0;
	links.links = NULL;

	/* Populate config */
	config.bolded = false;
	config.italicized = false;

	/* Populate lines */
	for (int i = 0; i < HISTORY_LINES; i++) lines.history[i] = NULL;
	for (int i = 0; i < READAHEAD_LINES; i++) lines.readahead[i] = NULL;

	/* Populate files */
	files.out = dest;
	files.in = src;

	/* Populate data */
	data.config = &config;
	data.lines = &lines;
	data.files = &files;
	data.links = &links;

	/* Populate lines.readahead[] */
	static size_t _;
	for (int i = 0; i < READAHEAD_LINES; i++)
	{
		_ = 0;
		if (getline(&lines.readahead[i], &_, files.in) == -1)
		{
			if (fgetc(files.in) != EOF)
				return perror("getline error"),
					   EXIT_FAILURE;
			else
				/* We've reached EOF, so break. */
				break;
		}
		if (strneql(lines.readahead[i], "---\n", 4))
		{
			/*
			 * We have reached the "---\n" that marks the end of our parsing.
			 * Mark the remaining lines as NULL. Don't proceed further.
			 */
			while (i < READAHEAD_LINES) lines.readahead[i++] = NULL;
			break;
		}
	}
	lines.curline = lines.readahead[0];

	/* Iterate over the lines */
	for (; lines.curline != NULL; get_next_line(&data))
	{
		for (int i = 0; i < (sizeof(LINEWISE_FUNCTIONS)/sizeof(LINEWISE_FUNCTIONS[0])); i++)
			if ((LINEWISE_FUNCTIONS[i])(&data))
				goto LINEWISE_FUNCTION_did_something;

		while (*lines.curline != '\0')
		{
			/* Check if somebody cares */
			for (int i = 0; i < (sizeof(CHARWISE_FUNCTIONS)/sizeof(CHARWISE_FUNCTIONS[0])); i++)
				if ((CHARWISE_FUNCTIONS[i])(&data))
					goto CHARWISE_FUNCTION_did_something;

			/* Nobody cares */
			fputc_escaped(*lines.curline, files.out);
			lines.curline++;

CHARWISE_FUNCTION_did_something:
			/* No need to lines.curline++, as it has already been done by the
			 * CHARWISE_FUNCTION who did something */
			continue;
		}

LINEWISE_FUNCTION_did_something:
		/* Skip everything else and proceed to next line */
		continue;
	}

	free_links_recursively(data.links);
	return EXIT_SUCCESS;
}

/*
 * IDEAS:
 *	- If link not found in data->links, then search readahead for it?
 */

/* vim:set noet nowrap ts=4 sts=4 sw=4 fdm=syntax: */
