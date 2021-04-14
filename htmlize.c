#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * ctype.h	- isalnum(), isalpha(), etc.
 * dirent.h	- opendir(), readdir()
 * stdio.h	- printf(), fopen(), fprintf(), etc
 * string.h	- str*(), mem*()
 */

#include "include/cd.h"
#include "include/date.h"
#include "include/stoi.h"
#include "constants.h"

#define date_to_text(x)		date_to_text(x, 0)
#define cd(x)				cd(x, argv)

const char INITIAL_HTML_PRE_SUBTITLE[] = "\
<html>\n\
    <head>\n\
        <title>%s</title>\n\
        %s\n\
        <link rel=\"stylesheet\" href=\"style.css\" media=\"screen\">\n\
<!--    <link rel=\"stylesheet\" href=\"recursive.css\" media=\"screen\">  -->\n\
    </head>\n\
    <body>\n\
        <header>\n\
            <h1 class=\"title\">\n\
                <span id=\"title\">%s</span>\n\
            </h1>\n\
        </header>\n\
        <p class=\"subtitle\">\n\
";
const char INITIAL_HTML_POST_SUBTITLE[] = "\
        </p>\n\
        <table class=\"blog-date\"><tr>\n\
                <td class=\"blog-date\">Date created</td>\n\
                <td class=\"blog-date\">%s</td>\n\
            </tr><tr>\n\
                <td class=\"blog-date\">Last modified</td>\n\
                <td class=\"blog-date\">%s</td>\n\
        </tr></table>\n\
        <main>\n\
<!-- Blog content starts here -->\n\
";

const char FINAL_HTML[] = "\
<!-- Blog content ends here -->\n\
        </main>\n\
        <br>\n\
        <br>\n\
        <br>\n\
%s\n\
    </body>\n\
</html>";


void fputc_escaped(char c, FILE *stream)
{
	switch (c)
	{
		case '<': fputs("&lt;",  stream); break;
		case '>': fputs("&gt;",  stream); break;
		case '&': fputs("&amp;", stream); break;
		default:  fputc(c,		 stream); break;
	}
}
void fputs_escaped(const char *s, FILE *stream)
{
	for (int i=0; s[i] != '\0'; i++)
		fputc_escaped(s[i], stream);
}


void htmlize(FILE *in, FILE *out)
/*
 * Things that are escaped using '\\' -
 *	- \```\n
 *	- \`code\`
 *	- \*bold\*
 *	- \_italic\_
 *	- \<HTML>
 *	- Table \| cells
 *	- \&#...; HTML Numeric char ref
 *	- \!(ID)[Link text\]
 */
/*
 * Things implemented -
 *	- ```
 *	- `code`
 *	- *bold*
 *	- _italic_
 *	- <table>
 *	- # Headings
 *	- Lists
 *	- HTML <tags>
 *	- Linebreak if two spaces at line end
 *	- &#...;  numeric character references
 *	- <br> at Blank lines with two spaces
 *	- Links
 */
{
	char TITLE[MAX_LINE_LENGTH];
	char DATE_CREATED[MAX_LINE_LENGTH];
	char DATE_MODIFIED[MAX_LINE_LENGTH];

	char line[MAX_LINE_LENGTH];
	char last_line[MAX_LINE_LENGTH];
	char links[MAX_LINKS + 1][MAX_LINE_LENGTH];	// +1 because indexing starts at 0


	/* ---- BEGIN initial HTML ---- */
	fgets(line, MAX_LINE_LENGTH, in);	// ---\n
	memmove(TITLE,		   fgets(line, MAX_LINE_LENGTH, in), MAX_LINE_LENGTH);
	memmove(DATE_CREATED,  fgets(line, MAX_LINE_LENGTH, in), MAX_LINE_LENGTH);
	memmove(DATE_MODIFIED, fgets(line, MAX_LINE_LENGTH, in), MAX_LINE_LENGTH);
	fgets(line, MAX_LINE_LENGTH, in);	// ---\n

	fputs("<!--\n", out);
	fprintf(out, "TITLE: %s", TITLE);
	fprintf(out, "CREATED: %s", DATE_CREATED);
	fprintf(out, "MODIFIED: %s", DATE_MODIFIED);
	fputs("-->\n", out);

	char *p;
	*(p = memchr(TITLE,			'\n', MAX_LINE_LENGTH)) = '\0';
	*(p = memchr(DATE_CREATED,	'\n', MAX_LINE_LENGTH)) = '\0';
	*(p = memchr(DATE_MODIFIED, '\n', MAX_LINE_LENGTH)) = '\0';

	fprintf(out, INITIAL_HTML_PRE_SUBTITLE, TITLE, FAVICON, TITLE);
	while (strcmp(fgets(line, MAX_LINE_LENGTH, in), "---\n"))  // ie. line != "---\n"
		fputs_escaped(line, out);
	fprintf(out, INITIAL_HTML_POST_SUBTITLE,
			date_to_text(DATE_CREATED),
			date_to_text(DATE_MODIFIED)
		   );
	/* ---- END initial HTML ---- */


	unsigned char BOLD_OPEN = 0;
	unsigned char ITALIC_OPEN = 0;
	unsigned char CODE_OPEN = 0;
	unsigned char CODEBLOCK_OPEN = 0;
	unsigned char HTML_TAG_OPEN = 0;
	unsigned char LINK_OPEN = 0;
	unsigned char LINK_TEXT_OPEN = 0;
	unsigned char TABLE_MODE = 0;
	unsigned char LIST_MODE = 0;

	unsigned int H_LEVEL = 0;

	char *pch;	// (p)revious (ch)aracter
	char *cch;	// (c)urrent  (ch)aracter
	char *nch;	// (n)ext     (ch)aracter

	for (;;)
	{
		// Save current line in last_line
		memmove(last_line, line, MAX_LINE_LENGTH);

		/*
		 * Read and store a line from *in into line[]
		 * Break loop if we've reached EOF (i.e. fgets() == NULL)
		 */
		if (fgets(line, MAX_LINE_LENGTH, in) == NULL)
			break;

		/*
		 * NOTE: Always remember, CODEBLOCKs take precedence over anything else
		 */

		// For <pre> blocks
		if (!memcmp(line, "\\```", 3))	// Escaped by a backslash
		{
			fputs("```\n", out);
			continue;
		}
		if (!memcmp(line, "```", 3))
		{
			++CODEBLOCK_OPEN;
			if (CODEBLOCK_OPEN %= 2)
				fputs("<pre>\n", out);
			else
				fputs("</pre>\n", out);
			continue;
		}
		if (CODEBLOCK_OPEN)
		{
			// Blindly escape everything and move on
			fputs_escaped(line, out);
			continue;
		}


		// End of blog
		if (!memcmp(line, "---\n", 4))
			break;


		// Link definition
		if (!memcmp(line, "\t[", 2))
		{
			int id;
			char *p;
			char *href;
			char *start;
			char *end;

			start = line + 2;						// +2 for the '\t['
			*(end = memchr(line, ']', MAX_LINE_LENGTH-1)) = '\0';	// for stoi()
			id = stoi(start);

			href = end + 2;							// +2 for the ']:'
			while (*href == ' ' || *href == '\t')	// Skip whitespaces, if any
				href++;

			/* Turn the trailing newline into string-terminator */
			*(p = memchr(href, '\n', MAX_LINE_LENGTH - (href-line))) = '\0';

			memmove(links[id], href, MAX_LINE_LENGTH - (href-line));
			continue;
		}


		// Blank line with two spaces
		if (!memcmp(line, "  \n", 3))
		{
			fputs("<br>\n", out);
			continue;
		}


		// Lists
		if (!memcmp(line, "<ul", 3) || !memcmp(line, "<ol", 3))
		{
			LIST_MODE = 1;
			fputs(line, out);
			continue;
		}
		if (!memcmp(line, "</ul", 4) || !memcmp(line, "</ol", 4))
		{
			LIST_MODE = 0;
			fputs(line, out);
			continue;
		}
		if (LIST_MODE)
		{
			char *p;
			if ((p = memchr(line, '-', MAX_LINE_LENGTH)) != NULL)
			{
				/*
				 * Check if the found '-' is the first char of the line
				 *
				 * ie. check whether there exists any non-blankspace character
				 * on this line before the '-' character we found
				 */
				char *i = p;
				while (i > line)
					if (!isblank(*--i))		// We found a non-blank character
						break;				// break out

				if (i == line)
					/*
					 * ie. we didn't "break" out of the previous loop
					 * That means our '-' *is* the first character of the line
					 * So, it should be turned into <li>
					 */
				{

					/* Move the text forward by 3 spaces to make space for the <li> tag */
					memmove(p+3, p, MAX_LINE_LENGTH-(p-line)-3);

					/* Add the <li> tag */
					memmove(p, "<li>", 4);	// 4, NOT 5, bcoz we DO NOT want the last \0

					/* Remove extra whitespaces, if any */
					p += 4;		// p now points to the char just after the <li> tag
					while (*p == ' ')
						memmove(p, p + 1, MAX_LINE_LENGTH-(p-line));
				}
			}
		}


		// Tables
		if (!memcmp(line, "<table", 6))
		{
			TABLE_MODE = 1;
			fputs(line, out);
			continue;
		}
		if (TABLE_MODE && !memcmp(line, "</table", 7))
		{
			TABLE_MODE = 0;
			fputs(line, out);
			continue;
		}


		// Headings (NOTE: starts from h2)
		if (line[0] == '#')
		{
			// Calculate H_LEVEL
			H_LEVEL = 1;
			while (line[H_LEVEL++] == '#') {;}

			// Remove leading #'s and spaces
			memmove(&line[0], &line[H_LEVEL - 1], MAX_LINE_LENGTH - (H_LEVEL - 1));	// #'s
			while (line[0] == ' ')									 				// spaces
				memmove(line, line + 1, MAX_LINE_LENGTH - 1);

			/*
			 * Create an ID for the heading
			 *	- Alphabets and numbers are kept as-is
			 *	- Spaces are transformed into '-'s
			 *	- Anything else is discarded
			 */
			char h_id[MAX_LINE_LENGTH] = "";
			for (int i=0,j=0; i < MAX_LINE_LENGTH; i++)
				if (line[i] == '\0')
					/*
					 * NOTE: because of the way we initialized h_id,
					 * all of the characters in the array are initialized to '\0'
					 * So, we don't need to add the \0 terminator here
					 */
				   	break;
				else if (isalnum(line[i]))
					h_id[j++] = line[i];
				else if (line[i] == ' ')
					h_id[j++] = '-';

			fprintf(
					out,
					"<h%i id=\"%s\"><a class=\"self-link\" href=\"#%s\">",
					H_LEVEL, h_id, h_id
				   );
		}


		// Line prefix for tables
		if (TABLE_MODE)
			fputs("<tr><td>", out);



		/* Initialize variables to avoid undefined behaviour */
		pch = line;
		cch = line;
		nch = line;
		/*
		 * Yes, I know the values are nonsense
		 * But at least it's _known_ nonsense
		 *
		 * If we access it while it's undefined, it will
		 *   - either SEGFAULT (in which case debugging it is easy)
		 *   - or *silently* input _unknown_ nonsense into outfile!
		 */

		/* iterate over the stored line[] */
		for (int index = 1; index < MAX_LINE_LENGTH; index++)
		{
			pch = cch;
			cch = nch;
			nch = line + index;

			if (*cch == '\0')	// we've reached end of string
				break;			// stop iterating


			if (*cch == '\\')
			{
				if (
						(*nch == '`' || *nch == '*' || *nch == '_')
						|| (!HTML_TAG_OPEN && *nch == '<')
						|| (TABLE_MODE && *nch == '|')
						|| (*nch == '&' && *(nch + 1) == '#')
						|| (!LINK_OPEN && *nch == '!' && *(nch + 1) == '(')
				   ) {;}
				else
					fputc('\\', out);
				continue;
			}


			// Heading tag close
			if (H_LEVEL && *cch == '\n')
			{
				fprintf(out, "</a></h%u>\n", H_LEVEL);
				H_LEVEL = 0;
				continue;
			}


			/*
			 * NOTE: CODE shall always come first
			 */

			// `code`
			if (!CODE_OPEN && *cch == '`' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
			{
				CODE_OPEN = 1;
				fputs("<code>", out);
				continue;
			}
			if (CODE_OPEN && *cch == '`' && *pch != '\\')
			{
				CODE_OPEN = 0;
				fputs("</code>", out);
				continue;
			}
			if (CODE_OPEN)
			{
				// Nothing to see here, just escape and move along
				fputc_escaped(*cch, out);
				continue;
			}


			// Linebreak if two spaces at line end
			if (*cch == '\n' && *pch == ' ' && *(pch-1) == ' ')
			{
				fputs("<br>\n", out);
				continue;
			}


			// HTML <tags>
			if ((*cch == '<') && (*nch == '/' || isalpha(*nch)) && *pch != '\\')
			{
				HTML_TAG_OPEN = 1;
				fputc('<', out);
				continue;
			}
			if (HTML_TAG_OPEN && *cch == '>')
			{
				HTML_TAG_OPEN = 0;
				fputc('>', out);
				continue;
			}
			if (HTML_TAG_OPEN)
			{
				// Nothing needs escaping, fputc() and move on
				fputc(*cch, out);
				continue;
			}


			// HTML Numeric Character references
			if (*cch == '&' && *nch == '#')
			{
				if (*pch == '\\')		// ie. it has been escaped
					fputs("&amp;", out);
				else
					fputc('&', out);
				continue;
			}


			// *bold*
			if (*cch == '*' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
			{
				++BOLD_OPEN;
				if (BOLD_OPEN %= 2)
					fputs("<strong>", out);
				else
					fputs("</strong>", out);
				continue;
			}


			// _italic_
			if (*cch == '_' && !(isalnum(*pch) && isalnum(*nch)) && *pch != '\\')
			{
				++ITALIC_OPEN;
				if (ITALIC_OPEN %= 2)
					fputs("<em>", out);
				else
					fputs("</em>", out);
				continue;
			}


			// Links	!(ID) attribute="value" [text]
			if (*cch == '!' && *nch == '(' && *pch != '\\' && !LINK_OPEN)
			{
				LINK_OPEN = 1;

				/* Mark string end for stoi() computation */
				char *p = memchr(nch, ')', MAX_LINE_LENGTH - index);
				*p = '\0';

				int link_id = stoi(nch + 1);	// +1 for '('
				fprintf(out, "<a href=\"%s\" ", links[link_id]);

				/* Skip the characters we have interpreted just now and continue looping */
				nch = p + 1;			// nch now point at char after the ')' in "!(id)"
				index = nch - line;		// since (nch = line + index)
				continue;
			}
			if (LINK_OPEN)
			{
				if (*cch == '[')
				{
					/*
					 * !(ID)     [Text]
					 * <a href=ID>Text</a>
					 *           ↑
					 *           |
					 */
					fputc('>', out);
					LINK_OPEN = 0;
					LINK_TEXT_OPEN = 1;
				}
				else
				{
					/*
					 * LINK_OPEN means we are inside the < and >
					 * So, we mustn't escape anything
					 */
					fputc(*cch, out);
				}
				continue;
			}
			if (LINK_TEXT_OPEN && *cch == ']' && *nch != '\\')
			{
				/*
				 * !(ID)     [Text]
				 * <a href=ID>Text</a>
				 *                ↑
				 *                |
				 */
				LINK_TEXT_OPEN = 0;
				fputs("</a>", out);
				continue;
			}


			// Table cells
			if (TABLE_MODE)
			{
				if (*cch == '|' && *pch != '\\')
				{
					/* Cell separator */
					fputs("</td><td>", out);
					continue;
				}
				if (*cch == '\n')
				{
					/* Table row ends here */
					fputs("</td></tr>\n", out);
					continue;
				}
			}


			// It's just a simple, innocent character
			fputc_escaped(*cch, out);
		}
	}


	fprintf(out, FINAL_HTML, FOOTER);
}


int main(int argc, const char **argv)
{
	DIR *dir;
	FILE *sfp;		// (s)ource      (f)ile (p)ointer
	FILE *dfp;		// (d)estination (f)ile (p)ointer
	struct dirent *dirent;

	if ((dir = opendir(SOURCE_DIR)) == NULL)
	{
		switch (errno)
		{
			case ENOENT:
				fprintf(stderr, "%s: directory not found: %s\n",	*argv, SOURCE_DIR);
				break;
			case ENOTDIR:
				fprintf(stderr, "%s: not a directory: %s\n",		*argv, SOURCE_DIR);
				break;
			case EACCES:
				fprintf(stderr, "%s: permission denied: %s\n",		*argv, SOURCE_DIR);
				break;
			default:
				fprintf(stderr, "%s: opendir error: %s\n",			*argv, SOURCE_DIR);
				break;
		}
		return 1;
	}

	while ((dirent = readdir(dir)) != NULL)
	{
		char *name = dirent->d_name;
		if (!memcmp(strrchr(name, '.'), SOURCE_EXT, strlen(SOURCE_EXT)))
		{
			/* Copy name to new_name */
			char new_name[FILENAME_MAX];
			memmove(new_name, name, strlen(name)+1);	// +1 for \0

			/* Change extension to .html */
			char *p = strrchr(new_name, '.');
			memmove(p, ".html", 6);		// 6, because ".html" has \0 at end

			/* Open source file */
			if (cd(SOURCE_DIR))
				return 1;
			sfp = fopen(name, "r");
			cd("..");

			/* Open destination file */
			if (cd(DEST_DIR))
				return 1;
			dfp = fopen(new_name, "w");
			cd("..");

			/* Process file content and close files  */
			htmlize(sfp, dfp);
			fclose(sfp);
			fclose(dfp);
			printf("%s -> %s\n", name, new_name);
		}
	}
	closedir(dir);
}

// vim:noet:ts=4:sts=0:sw=0:fdm=syntax
