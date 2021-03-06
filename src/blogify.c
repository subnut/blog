#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * dirent.h	- opendir(), readdir()
 * errno.h	- if opendir() fails, show proper error msg
 * stdio.h	- printf(), fopen(), fprintf(), etc
 * string.h	- str*(), mem*()
 */

#include "constants.h"
#include "include/cd.h"
#include "include/date_to_text.h"
#include "include/escape.h"
#include "include/htmlize.h"

#define cd(x) \
        cd(x, argv)


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
	char TITLE[MAX_LINE_LENGTH];
	char DATE_CREATED[MAX_LINE_LENGTH];
	char DATE_MODIFIED[MAX_LINE_LENGTH];
	char BUFFER[MAX_LINE_LENGTH];

	fgets(TITLE,         MAX_LINE_LENGTH, in);
	fgets(DATE_CREATED,  MAX_LINE_LENGTH, in);
	fgets(DATE_MODIFIED, MAX_LINE_LENGTH, in);
	fgets(BUFFER,        MAX_LINE_LENGTH, in);	// ---\n

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
	htmlize(in, out);	// htmlize the subtitle text

	/*
	 * NOTE: This will not work -
	 *
	 * 		char buffer[15];
	 *
	 * 		fprintf(out, INITIAL_HTML_POST_SUBTITLE,
	 * 				date_to_text(DATE_CREATED, buffer),
	 * 				date_to_text(DATE_MODIFIED, buffer)
	 * 			   );
	 *
	 * Why? Because both date_to_text() invocations shall return a
	 * pointer to the same buffer, and both shall operate on that same buffer.
	 *
	 * So, when fprintf() starts formatting the string, it finds the buffer's
	 * value to be what the last invocation of date_to_text() had put in it.
	 * (ie. the string form of DATE_MODIFIED)
	 *
	 * This will cause both "Date created" and "Last modified" table fields to
	 * show the value of DATE_MODIFIED.
	 *
	 *
	 * The solution?
	 * Use different buffers for DATE_CREATED and DATE_MODIFIED
	 */

	char DATE_CREATED_str[15];
	char DATE_MODIFIED_str[15];
	fprintf(out, INITIAL_HTML_POST_SUBTITLE,
			date_to_text(DATE_CREATED, DATE_CREATED_str),
			date_to_text(DATE_MODIFIED, DATE_MODIFIED_str)
		   );
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
			process_file(sfp, dfp);
			fclose(sfp);
			fclose(dfp);

#ifdef PRINT_FILENAMES
			printf("%s -> %s\n", name, new_name);
#endif /* PRINT_FILENAMES */
		}
	}
	closedir(dir);
}

// vim:noet:ts=4:sts=0:sw=0:fdm=syntax
