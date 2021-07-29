#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * ctype.h	- isalnum(), isalpha(), etc.
 * stbool.h	- bool, true, false
 * stdio.h	- printf(), fopen(), fprintf(), etc
 * string.h	- str*(), mem*()
 */

#include "constants.h"
#include "include/charref.h"
#include "include/debug.h"
#include "include/escape.h"
#include "include/htmlize.h"
#include "include/stoi.h"
#include "include/urlencode.h"


/*
 * A macro to get the number of characters remaining in the string.
 * Useful for safely incrementing ptr->line without going out-of-bounds.
 * Or for doing memcmp().
 *
 * (MAX_LINE_LENGTH - 1)		Total number of indices
 * (ptr->line - ptr->readahead[0])	Current index
 *
 * - 1	'cause the last index is occupied by the trailing '\0'
 */
#define REMAINING_CHARS \
	((int)((MAX_LINE_LENGTH - 1) - (ptr->line - ptr->readahead[0]) - 1))


static void shift_lines         (struct data *);
static void get_next_line       (struct data *);
static void parse_line          (struct data *);
static int  CODEBLOCK           (struct data *);
static int  HEADINGS            (struct data *);


/**** [START] Utility functions ****/
static inline void
toggle(bool *val)
{
	if (*val)
		*val = false;
	else
		*val = true;
}

static void
shift_lines(struct data *ptr)
{
	for (int i = HISTORY - 1; i >= 0; i--)
		memmove(ptr->history[i], ptr->history[i - 1], MAX_LINE_LENGTH);
	memmove(ptr->history[0], ptr->readahead[0], MAX_LINE_LENGTH);	// Copy to history
	for (int i = 1; i < READAHEAD; i++)
		memmove(ptr->readahead[i - 1], ptr->readahead[i], MAX_LINE_LENGTH);
	ptr->line = ptr->readahead[0];	// Reset ptr->line
}

static void
get_next_line(struct data *ptr)
{
	shift_lines(ptr);

	/*
	 * Iterate over ptr->readahead and check if we've already reached the
	 * end of the blog before.  If we've already reached the end of blog
	 * before, then don't fgets() anymore.
	 *
	 * This helps avoid the situation where we read more than we require.
	 * Eg. Say the file descriptor points to a file like this -
	 *	| Lorem ipsum
	 *	| Ipsum Dolor
	 *	| ---
	 *	| Sit amet
	 *	| Amet foobar
	 * We need to read only upto the third line, and then exit. But, say
	 * READAHEAD is set to 2. What happens? We read upto the 5th line!
	 * And then, if any function uses the file descriptor again, it gets an
	 * EOF, whereas it should have rightfully gotten the 4th line.
	 */
	for (int i = READAHEAD - 1; i > 0; i--)
		if (ptr->readahead[i][0] == '\0')
		{
			/* We've already reached the end of blog.
			 * Mark next buffer as empty. */
			ptr->readahead[READAHEAD][0] = '\0';
			return;
		}

	/* Read a new line. If NULL, that means EOF, so mark buffer empty */
	if (fgets(ptr->readahead[READAHEAD-1], MAX_LINE_LENGTH, ptr->files->src) == NULL)
		ptr->readahead[READAHEAD-1][0] = '\0';	// Mark buffer as empty

	/* If current line marks End of blog, then mark buffer empty */
	if (!memcmp(ptr->readahead[READAHEAD-1], "---\n", 5))
		ptr->readahead[READAHEAD-1][0] = '\0';	// Mark buffer as empty
}
/**** [END] Utility functions ****/


/**** [START] Line-wise functions ****/
static int
LISTS(struct data *ptr)	// XXX: MAX_LINE_LENGTH dependent
{
	/* YAGNI
	if (ptr->line[0] == '\\')
		if (!memcmp(ptr->line + 1, "<ul", 3) || !memcmp(ptr->line + 1, "<ol", 3))
		{
			ptr->line++;
			return 0;
		}
	*/

	if (ptr->line[0] != '<')
		return 1;

	if ((memcmp(ptr->line, "<ul", 3) && memcmp(ptr->line, "<ol", 3)))
		return 1;

	fputs(ptr->line, ptr->files->dest);
	get_next_line(ptr);
	while (ptr->line[0] != '\0' && (memcmp(ptr->line, "</ul", 4) && memcmp(ptr->line, "</ol", 4)))
	{
		char *hyphen;
		if ((hyphen = memchr(ptr->line, '-', MAX_LINE_LENGTH)) != NULL)
		{
			while (ptr->line <= hyphen)
				if (ptr->line[0] == ' ')
				{
					fputc(' ', ptr->files->dest);
					ptr->line++;
				}
				else break;
			if (ptr->line == hyphen)
			{
				fputs("<li>", ptr->files->dest);
				ptr->line++;
				/* Skip over the whitespaces, if any */
				while (ptr->line[0] == ' ')
					ptr->line++;
			}
			else if (ptr->line == hyphen - 1 && *ptr->line == '\\')
				ptr->line++;
		}

		parse_line(ptr);
		if (ptr->line[0] == '\n')
			fputc('\n', ptr->files->dest);

		get_next_line(ptr);
	}
	fputs(ptr->line, ptr->files->dest);
	return 0;

}

static int
CODEBLOCK(struct data *ptr)
{
	if (!memcmp(ptr->line, "\\```", 4))
	{
		fputs("```\n", ptr->files->dest);
		return 0;
	}
	if (!memcmp(ptr->line, "```", 3))
	{
		fputs("<pre>\n", ptr->files->dest);
		get_next_line(ptr);
		while (memcmp(ptr->line, "```", 3) && memcmp(ptr->line, "", 1))
		{
			if (!memcmp(ptr->line, "\\```", 4))
			{
				ptr->line += 4;
				fputs("```", ptr->files->dest);
			}
			fputs_escaped(ptr->line, ptr->files->dest);
			get_next_line(ptr);
		}
		fputs("</pre>\n", ptr->files->dest);
		return 0;
	}
	return 1;
}

static int
FOOTNOTES(struct data *ptr)
{
	if (!memcmp(ptr->line, "\\^^^\n", 5))
	{
		fputs("^^^\n", ptr->files->dest);
		return 0;
	}
	if (!memcmp(ptr->line, "^^^\n", 4))
	{
		fputs("<p id=\"footnotes\">\n", ptr->files->dest);
		for (get_next_line(ptr); ptr->line[0] != '\0'; get_next_line(ptr))
		{
			char *p;
			if ((p = memchr(ptr->line, ':', REMAINING_CHARS)) != NULL)
			{
				*p = '\0';
				fprintf(ptr->files->dest,
						"<a class=\"footnote\" id=\"fn:%s\" href=\"#fnref:%s\">[%s]</a>",
						ptr->line, ptr->line, ptr->line
				       );
				*p = ':';
				ptr->line = p;
			}
			parse_line(ptr);
			if (ptr->line[0] == '\n')
				fputs("<br>\n", ptr->files->dest);
		}
		return 0;
	}
	return 1;
}
/**** [END] Line-wise functions ****/


/**** [START] Character-wise functions ****/
static int
DUALSPACEBREAK(struct data *ptr)
{
	if (ptr->line[0] != ' ') return 1;
	switch (REMAINING_CHARS)
	{
		case 1:
			{
				if (ptr->readahead[1][0] != ' ')
					return 1;
				if (ptr->readahead[1][1] != '\n')
					return 1;
				break;
			}
		case 2:
			{
				if (ptr->line[1] != ' ')
					return 1;
				if (ptr->readahead[1][0] != '\n')
					return 1;
				ptr->line++;
				break;
			}
		default:
			{
				if (ptr->line[1] != ' ')
					return 1;
				if (ptr->line[2] != '\n')
					return 1;
				ptr->line += 2;
				break;
			}
	}
	fputs("<br>", ptr->files->dest);
	return 0;
}

static int
HEADINGS(struct data *ptr)
{
	/* If we aren't on the first character of the line, it ain't our job */
	if (ptr->line != ptr->readahead[0])
		return 1;

	/* Huh? First character isn't a '#'? Hmm... */
	if (ptr->line[0] != '#')
	{
		/* Has somebody escaped the heading using backslashes? */
		if (ptr->line[0] == '\\' && ptr->line[1] == '#')
		{
			ptr->line++;
			return 0;	// We did our job
		}
		else return 1;	// Not our business
	}

	static int H_LEVEL;
	H_LEVEL = 0;
	while (ptr->line[0] == '#')
	{
		H_LEVEL++;
		ptr->line++;
	}
	while (ptr->line[0] == ' ')
		ptr->line++;

	/*
	 * Create an ID for the heading
	 *	- Alphabets and numbers are kept as-is
	 *	- Spaces are transformed into '-'s
	 *	- Anything else is discarded
	 */
	static char h_id[MAX_LINE_LENGTH];
	for (int i=0; i < MAX_LINE_LENGTH; i++)
		h_id[i] = '\0';
	{
		/* The enclosing braces ensure that the variables declared
		 * inside them don't leak out of this scope (ie. they don't
		 * leak out of the enclosing braces) */
		char *line;
		int index;
		index = 0;
		line = ptr->readahead[index++];
		for (int i=0,j=0; line[i] != '\n'; i++)
			if (isalnum(line[i]))
				h_id[j++] = line[i];
			else if (line[i] == ' ')
				h_id[j++] = '-';
			else if (line[i] == '\0')
				line = ptr->readahead[index++];
	}

	/* Print the opening HTML tags */
	fprintf(ptr->files->dest,
			"<h%i id=\"%s\"><a class=\"self-link\" href=\"#%s\">",
			H_LEVEL, h_id, h_id);

	/* Parse the remaining of the line */
	parse_line(ptr);
	while (ptr->line[0] != '\n')
	{
		get_next_line(ptr);
		parse_line(ptr);
	}

	/* Print the closing HTML tags */
	fprintf(ptr->files->dest, "</a></h%u>", H_LEVEL);
	return 0;
}

static int
CHARREFS(struct data *ptr)	// XXX: MAX_LINE_LENGTH dependent
{

	char *line;
	char *end;

	/* It's either "&...;" or "\&...;".
	 * So, if we can't find '&' and ';', it's not a charref */
	if ((end = memchr(ptr->line, ';', REMAINING_CHARS)) == NULL)
		return 1;
	if (ptr->line[0] == '&')
		line = ptr->line;
	else if (ptr->line[0] == '\\' && ptr->line[1] == '&')
		line = ptr->line + 1;
	else
		return 1;

	if (!is_charref(line, REMAINING_CHARS))
	{
		for (char *p = line; p <= end; p++)
			if(ptr->line[0] == '\\')
				fputc_escaped(*p, ptr->files->dest);
			else
				fputc(*p, ptr->files->dest);

		/*
		 * ptr->line = end + 1  since we've already fputc'd *end in the
		 * above loop
		 */
		ptr->line = end + 1;
		return 0;
	}
	return 1;
}

static int
HTML_TAGS(struct data *ptr)
{
	if (ptr->line[0] != '<')
	{
		if (ptr->line[0] == '\\' && ptr->line[1] == '<' &&
				(REMAINING_CHARS > 1
				 ? (isalpha(ptr->line[2]) || ptr->line[2] == '/')
				 : (isalpha(ptr->readahead[1][0]) || ptr->readahead[1][0] == '/')
				))
		{
			ptr->line++;	// for '\'
			ptr->line++;	// for '<'
			fputs("&lt;", ptr->files->dest);
			while (ptr->line[0] != '>')
			{
				if (ptr->line[0] == '\0')
					get_next_line(ptr);
				if (ptr->line[0] == '\0')
					break;

				fputc_escaped(ptr->line[0], ptr->files->dest);
				ptr->line++;
			}
			fputs("&gt;", ptr->files->dest);
			ptr->line++;
			return 0;	// We did our job
		}
		else return 1;	// Not our business
	}

	/* The character right after the < MUST be isalpha() or '/' */
	if (ptr->line[1] != '/' && !isalpha(ptr->line[1]))
		return 1;

	fputc('<', ptr->files->dest);
	ptr->line++;
	while (ptr->line[0] != '>')
	{
		if (ptr->line[0] == '\0')
			get_next_line(ptr);
		if (ptr->line[0] == '\0')
			break;

		fputc(ptr->line[0], ptr->files->dest);
		ptr->line++;
	}
	fputc('>', ptr->files->dest);
	ptr->line++;
	return 0;
}

static int
CODE(struct data *ptr)
{
	if (ptr->line[0] != '`')
	{
		if (ptr->line[0] == '\\' && ptr->line[1] == '`')
		{
			ptr->line++;
			return 0;	// We did our job
		}
		else return 1;	// Not our business
	}

	/* Check if the ` was escaped */
	if (ptr->line != ptr->readahead[0] && ptr->line[-1] == '\\')
	{
		fputc('`', ptr->files->dest);
		ptr->line++;
		return 0;
	}

	fputs("<code>", ptr->files->dest);
	ptr->line++;
	while (1)
	{
		if (ptr->line[0] == '\0')
			get_next_line(ptr);
		if (ptr->line[0] == '\0')
			break;

		if (ptr->line[0] != '`')
		{
			if (ptr->line[0] == '\\' && ptr->line[1] == '`')
			{
				fputc('`', ptr->files->dest);
				ptr->line += 2;
			}
			else
			{
				fputc_escaped(ptr->line[0], ptr->files->dest);
				ptr->line++;
			}
			continue;
		}
		break;
	}
	fputs("</code>", ptr->files->dest);
	ptr->line++;
	return 0;
}

static int
BOLD(struct data *ptr)
{
	if (ptr->line[0] != '*')
	{
		if (ptr->line[0] == '\\' && ptr->line[1] == '*')
		{
			ptr->line++;
			return 0;	// We did our job
		}
		else return 1;	// Not our business
	}

	/* Check if the * was escaped */
	if (ptr->line != ptr->readahead[0] && ptr->line[-1] == '\\')
	{
		fputc('*', ptr->files->dest);
		ptr->line++;
		return 0;
	}

	if (ptr->config->BOLD_OPEN)
		fputs("</strong>", ptr->files->dest);
	else
		fputs("<strong>", ptr->files->dest);
	toggle(&ptr->config->BOLD_OPEN);
	ptr->line++;
	return 0;
}

static int
ITALIC(struct data *ptr)
{
	if (ptr->line[0] != '_')
	{
		if (ptr->line[0] == '\\' && ptr->line[1] == '_')
		{
			ptr->line++;
			return 0;	// We did our job
		}
		else return 1;	// Not our business
	}

	/* Check if the * was escaped */
	if (ptr->line != ptr->readahead[0] && ptr->line[-1] == '\\')
	{
		fputc('_', ptr->files->dest);
		ptr->line++;
		return 0;
	}

	if (ptr->config->ITALIC_OPEN)
		fputs("</em>", ptr->files->dest);
	else
		fputs("<em>", ptr->files->dest);
	toggle(&ptr->config->ITALIC_OPEN);
	ptr->line++;
	return 0;
}

static int
FOOTNOTE(struct data *ptr)	// XXX: MAX_LINE_LENGTH dependent
{
	if (ptr->line[0] != '[')
	{
		if (ptr->line[0] == '\\' && REMAINING_CHARS > 2 && !memcmp(ptr->line + 1, "[^", 2))
		{
			fputs_escaped("[^", ptr->files->dest);
			ptr->line += 3;
			return 0;	// We did our job
		}
		else return 1;	// Not our business
	}

	/* Check if the [^ was escaped */
	if (ptr->line != ptr->readahead[0] && ptr->line[-1] == '\\')
	{
		fputs_escaped("[^", ptr->files->dest);
		ptr->line += 3;
		return 0;
	}

	char *p;
	p = memchr(ptr->line, ']', REMAINING_CHARS);
	*p++ = '\0';
	ptr->line += 2;	// 2 for "[^"

	fprintf(ptr->files->dest,
			"<a class=\"footnote\" id=\"fnref:%s\" href=\"#fn:%s\"><sup>[%s]</sup></a>",
			ptr->line, ptr->line, ptr->line
	       );

	ptr->line = p;
	return 0;
}

static void
parse_line(struct data *ptr)
{
	while (ptr->line[0] != '\n')
	{
		/* If we hit the end of the string, get_next_line */
		if (ptr->line[0] == '\0')
			get_next_line(ptr);
		/* If we still have '\0', that means we've got EOF */
		if (ptr->line[0] == '\0')
			break;

		/* Check if the current character is worth anything to anyone */
		if (
				   !HEADINGS(ptr)
				|| !CHARREFS(ptr)
				|| !HTML_TAGS(ptr)
				|| !CODE(ptr)
				|| !BOLD(ptr)
				|| !ITALIC(ptr)
				|| !FOOTNOTE(ptr)
				|| !DUALSPACEBREAK(ptr)
				|| !1 // XXX: Replace the 1 with any new function
		   )
			continue;

		/* Nobody cared. fputc_escaped() it and move on. */
		fputc_escaped(ptr->line[0], ptr->files->dest);
		ptr->line++;
	}
}
/**** [END] Character-wise functions ****/


int
htmlize(FILE *src, FILE *dest)
/*
 * Things that are escaped using '\\' -
 *	- \```
 *	- \`code\`
 *	- \*bold\*
 *	- \_italic\_
 *	- \# Headings
 *	- \<HTML tags>
 *	- \&...; HTML Char refs
 *	- Footnotes\[^10]
 *	- XXX: Lists are NOT escapable
 *	- TODO: Table \| cells
 *	- TODO: \!(ID)[Link text\]
 */
/*
 * Things implemented -
 *	- ```
 *	- `code`
 *	- *bold*
 *	- _italic_
 *	- # Headings
 *	- HTML <tags>
 *	- HTML Character references
 *	- Lists
 *	- Linebreak if two spaces at line end
 *	- Footnotes[^10]
 *	- TODO: <table>
 *	- TODO: Links
 *	- <br> at Blank lines with two spaces (FIXME: Support for automatic paragraphs, without two blankspaces)
 */
{
	if (MAX_LINE_LENGTH < 5)
	{
		/*
		 * To mark the end of current input, we need to read the string
		 * "---\n" into the buffer. Since "---\n" is actually the array
		 * {'-', '-', '-', '\n', '\0'}, which contains 5 elements, we
		 * need to have MAX_LINE_LENGTH >= 5
		 *
		 * Since we don't, BAIL OUT!
		 */
		fputs("htmlize(): MAX_LINE_LENGTH too low", stderr);
		return -1;
	}

	struct data data;
	struct data *ptr;
	ptr = &data;

	struct files files;
	data.files	= &files;
	files.src	= src;
	files.dest	= dest;

	struct config config;
	data.config		= &config;
	config.BOLD_OPEN	= false;
	config.ITALIC_OPEN	= false;
	config.LINK_OPEN	= false;
	config.LINK_TEXT_OPEN	= false;
	config.TABLE_MODE	= false;

	/* Initialize with empty lines */
	for (int i = 0; i < READAHEAD; i++)
		for (int j = 0; j < MAX_LINE_LENGTH; j++)
			data.readahead[i][j] = '\0';
	for (int i = 0; i < HISTORY; i++)
		for (int j = 0; j < MAX_LINE_LENGTH; j++)
			data.history[i][j] = '\0';

	/* Populate data.readahead */
	for (int i = 0; i < READAHEAD; i++)
	{
		if (fgets(data.readahead[i], MAX_LINE_LENGTH, src) == NULL)
			break;
		if (!memcmp(data.readahead[i], "---\n", 5))
		{
			data.readahead[i][0] = '\0';	// Mark as empty
			break;
		}
	}

	/* If the first line we read is empty, that means we hit EOF as-soon-as
	 * we started reading. ie. We received no input. */
	if (!memcmp(data.readahead[0], "", 1))
		return 1;	// No input

	ptr->line = ptr->readahead[0];
	while (ptr->line[0] != '\0')
	{
		if (!memcmp(ptr->line, "\\---\n", 6))
		{
			fputs("---\n", ptr->files->dest);
			get_next_line(ptr);
			continue;
		}

		/* Check if the current line is worth anything to anyone */
		if (
				   !CODEBLOCK(ptr)
				|| !FOOTNOTES(ptr)
				|| !LISTS(ptr)
				|| !1 // XXX: Replace the 1 with any new function
		   ) {;}
		else
		{
			parse_line(ptr);
			if (ptr->line[0] == '\n')
				fputc('\n', ptr->files->dest);
		}
		get_next_line(ptr);
	}
	return 0;
}

// 		// Link definition
// 		if (!memcmp(line, "\t[", 2))
// 		{
// 			int id;
// 			char *p;
// 			char *href;
// 			char *start;
// 			char *end;
// 
// 			start = line + 2;	// +2 for the '\t['
// 			*(end = memchr(line, ']', MAX_LINE_LENGTH-1)) = '\0'; // mark the end
// 			id = stoi(start);
// 
// 			if (id > MAX_LINKS)
// 			{
// 				fprintf(stderr,
// 						"link index %i is bigger than MAX_LINKS (%i)",
// 						id, MAX_LINKS);
// 				continue;
// 			}
// 
// 			href = end + 2;							// +2 for the ']:'
// 			while (*href == ' ' || *href == '\t')	// Skip whitespaces, if any
// 				href++;
// 
// 			/* Turn the trailing newline into string-terminator */
// 			*(p = memchr(href, '\n', MAX_LINE_LENGTH - (href-line))) = '\0';
// 
// 			memmove(links[id], href, MAX_LINE_LENGTH - (href-line));
// 			continue;
// 		}
// 
// 
// 		// Tables
// 		if (!memcmp(line, "<table", 6))
// 		{
// 			TABLE_MODE = 1;
// 			fputs(line, out);
// 			continue;
// 		}
// 		if (TABLE_MODE && !memcmp(line, "</table", 7))
// 		{
// 			TABLE_MODE = 0;
// 			fputs(line, out);
// 			continue;
// 		}
// 
// 
// 		// Line prefix for tables
// 		if (TABLE_MODE)
// 			fputs("<tr><td>", out);
// 
// 
// 		/* iterate over the stored line[] */
// 		for (int index = 1; index < MAX_LINE_LENGTH; index++)
// 		{
// 			pch = cch;
// 			cch = nch;
// 			nch = line + index;
// 
// 			if (*cch == '\0')	// we've reached end of string
// 				break;			// stop iterating
// 
// 
// 			if (*cch == '\\')
// 			{
// 				if (
// 						(*nch == '`' || *nch == '*' || *nch == '_')
// 						|| (!HTML_TAG_OPEN && *nch == '<')
// 						|| (TABLE_MODE && *nch == '|')
// 						|| (!LINK_OPEN && *nch == '!' && *(nch + 1) == '(')
// 						|| (*nch == '&' && (*(nch + 1) == '#' || is_named_charref(nch)))
// 						|| (*nch == '[' && *(nch + 1) == '^')
// 				   ) {;} // print nothing
// 				else
// 					fputc('\\', out);
// 				continue;
// 			}
// 
// 
// 			// Linebreak if two spaces at line end
// 			if (*cch == '\n' && *pch == ' ' && *(pch-1) == ' ')
// 			{
// 				fputs("<br>\n", out);
// 				continue;
// 			}
// 
// 
// 			// Links	!(ID) attribute="value" [text]
// 			if (*cch == '!' && *nch == '(' && *pch != '\\' && !LINK_OPEN)
// 			{
// 				LINK_OPEN = 1;
// 
// 				/* Mark string end for stoi() computation */
// 				char *p = memchr(nch, ')', MAX_LINE_LENGTH - index);
// 				*p = '\0';
// 
// 				int link_id = stoi(nch + 1);	// +1 for '('
// 				if (link_id > MAX_LINKS)
// 				{
// 					fprintf(stderr,
// 							"link index %i is bigger than MAX_LINKS (%i)",
// 							link_id, MAX_LINKS);
// 					continue;
// 				}
// 				else
// 					fprintf(out, "<a href=\"%s\" ", links[link_id]);
// 
// 				/* Skip the characters we have interpreted just now and continue looping */
// 				nch = p + 1;			// nch now point at char after the ')' in "!(id)"
// 				index = nch - line;		// since (nch = line + index)
// 				continue;
// 			}
// 			if (LINK_OPEN)
// 			{
// 				if (*cch == '[')
// 				{
// 					/*
// 					 * !(ID)     [Text]
// 					 * <a href=ID>Text</a>
// 					 *           ↑
// 					 *           |
// 					 */
// 					fputc('>', out);
// 					LINK_OPEN = 0;
// 					LINK_TEXT_OPEN = 1;
// 				}
// 				else
// 				{
// 					/*
// 					 * LINK_OPEN means we are inside the < and >
// 					 * So, we mustn't escape anything
// 					 */
// 					fputc(*cch, out);
// 				}
// 				continue;
// 			}
// 			if (LINK_TEXT_OPEN && *cch == ']' && *nch != '\\')
// 			{
// 				/*
// 				 * !(ID)     [Text]
// 				 * <a href=ID>Text</a>
// 				 *                ↑
// 				 *                |
// 				 */
// 				LINK_TEXT_OPEN = 0;
// 				fputs("</a>", out);
// 				continue;
// 			}
// 
//
// 			// Table cells
// 			if (TABLE_MODE)
// 			{
// 				if (*cch == '|' && *pch != '\\')
// 				{
// 					/* Cell separator */
// 					fputs("</td><td>", out);
// 					continue;
// 				}
// 				if (*cch == '\n')
// 				{
// 					/* Table row ends here */
// 					fputs("</td></tr>\n", out);
// 					continue;
// 				}
// 			}
// 
// 		}
// 	}
// }

// vim:fdm=syntax:sw=8:sts=8:ts=8:nowrap:
