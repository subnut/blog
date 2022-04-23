#define _POSIX_C_SOURCE 200809L

#include "constants.h"
#include "include/proto/date_to_text.h"
#include "include/proto/escape.h"
#include "include/proto/htmlize.h"

#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * _POSIX_C_SOURCE	- getline, strdup
 * dirent.h			- opendir, readdir
 * stdio.h			- getline, perror, printf, fopen, fprintf, etc
 * stddef.h			- NULL
 * stdlib.h			- exit, EXIT_FAILURE
 * string.h			- strrchr, strlen
 * unistd.h			- chdir
 */

#include "include/defs/free.h"
#include "include/defs/perror.h"
#include "include/defs/streql.h"
#include "include/defs/stringify.h"

#define cd(dir) \
	if (chdir(dir)) \
	return perror("chdir error"), \
	EXIT_FAILURE

static const char INITIAL_HTML_PRE_SUBTITLE[] = "\
<html>\n\
    <head>\n\
        <meta charset=\"utf-8\"/>\n\
        <title>%s</title>\n\
        %s\n\
        <link rel=\"stylesheet\" href=\"style.css\" media=\"screen\">\n\
        <link rel=\"stylesheet\" href=\"recursive.css\" media=\"screen\">\n\
        <link rel=\"stylesheet\" href=\"print.css\" media=\"print\">\n\
    </head>\n\
    <body>\n\
        <header>\n\
            <h1 class=\"blog-title\">%s</h1>\n\
        </header>\n\
        <div id=\"wrapper\">\n\
            <p class=\"subtitle\">\n\
";
static const char INITIAL_HTML_POST_SUBTITLE[] = "\
            </p>\n\
            <p class=\"blog-date\">\n\
                Published on %s.\n\
                Last modified on %s.\n\
            </p>\n\
            <main>\n\
<!-- Blog content starts here -->\n\
";

static const char FINAL_HTML[] = "\
<!-- Blog content ends here -->\n\
            </main>\n\
            %s\n\
        </div>\n\
    </body>\n\
</html>\n\
";

static void
initial_html(FILE *in, FILE *out)
{
	// Ephermal variables
	char *p;
	size_t _;

	// Valuable variables
	char *TITLE;
	char *DATE_CREATED;
	char *DATE_MODIFIED;

#define getline(a) \
	if (getline(a, (_=0, &_), in) == -1) \
	exit((perror("getline error"), EXIT_FAILURE))

	getline(&TITLE);
	getline(&DATE_CREATED);
	getline(&DATE_MODIFIED);

#undef getline

	/* Fetch the remaining "---\n" */
	char BUFFER[5];
	fgets(BUFFER, 5, in);

	fputs("<!--\n", out);
	fprintf(out, "TITLE: %s",    TITLE);
	fprintf(out, "CREATED: %s",  DATE_CREATED);
	fprintf(out, "MODIFIED: %s", DATE_MODIFIED);
	fputs("-->\n", out);

	*(p = strrchr(TITLE,         '\n')) = '\0';
	*(p = strrchr(DATE_CREATED,  '\n')) = '\0';
	*(p = strrchr(DATE_MODIFIED, '\n')) = '\0';

	fprintf(out, INITIAL_HTML_PRE_SUBTITLE, TITLE, FAVICON, TITLE);
	htmlize(in, out);	// htmlize the subtitle text

	fprintf(out, INITIAL_HTML_POST_SUBTITLE,
			date_to_text(DATE_CREATED),
			date_to_text(DATE_MODIFIED)
		   );

	free(TITLE);
	free(DATE_CREATED);
	free(DATE_MODIFIED);
}

static void
process_file(FILE *src, FILE *dest)
{
	initial_html(src, dest);
	htmlize(src, dest);
	fprintf(dest, FINAL_HTML, FOOTER);
}

int
main(int argc, const char **argv)
{
	DIR *dir;
	FILE *sfp;		// (s)ource		 (f)ile (p)ointer
	FILE *dfp;		// (d)estination (f)ile (p)ointer
	struct dirent *dirent;

	if ((dir = opendir(STRINGIFY(BLOG_SRCDIR))) == NULL)
		return perror("opendir error"),
			   EXIT_FAILURE;

	while ((dirent = readdir(dir)) != NULL)
	{
		char *name = dirent->d_name;
		if (streql(strrchr(name, '.'), STRINGIFY(BLOG_EXT)))
		{
			/* Copy name to new_name */
			char *new_name;
			if ((new_name = strdup(name)) == NULL)
				return perror("strdup error"),
					   EXIT_FAILURE;

			/* Ensure enough space available */
			if (strlen(STRINGIFY(BLOG_EXT)) < strlen(".html"))
				if ((new_name = realloc(name,
								sizeof(new_name[0]) * (
									  strlen(new_name)
									- strlen(STRINGIFY(BLOG_EXT))
									+ strlen(".html")
									+ 1	// Trailing '\0'
									)
								)
					) == NULL)
					return perror("realloc error"),
						   EXIT_FAILURE;

			/* Change extension to .html */
			char *p = strrchr(new_name, '.');
			memmove(p, ".html", 6);		// 6, because ".html" has \0 at end

#define fileopen(fp, name, mode)	\
			if ((fp = fopen(name, mode)) == NULL) \
				return perror("fopen failed"), EXIT_FAILURE;

			/* Open source file */
			cd(STRINGIFY(BLOG_SRCDIR));
			fileopen(sfp, name, "r");
			cd("..");

			/* Open destination file */
			cd(STRINGIFY(HTML_DESTDIR));
			fileopen(dfp, new_name, "w");
			cd("..");

			/* Process file content and close files  */
			process_file(sfp, dfp);
			fclose(sfp);
			fclose(dfp);

			/* Free allocated memory */
			free(new_name);
		}
	}
	closedir(dir);
}

/* vim: set noet nowrap fdm=syntax: */
