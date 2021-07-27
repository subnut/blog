/*
 * TODO: Allow HTML character references in TITLE
 * (tip: use "include/charref.h")
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>

/*
 * stdio.h  - fopen, fclose
 * string.h - memcmp, memchr
 * dirent.h - opendir, readdir
 */

#include "constants.h"
#include "include/cd.h"
#include "include/date_to_text.h"
#include "include/escape.h"
#include "include/stoi.h"
#include "include/urlencode.h"

#define cd(x) \
        cd(x, argv)

static const char INITIAL_TEXT[] = "\
<html>\n\
    <head>\n\
        <meta charset=\"utf-8\"/>\n\
        <title>subnut's blog</title>\n\
        %s\n\
        <link rel=\"stylesheet\" href=\"style.css\" media=\"screen\">\n\
        <link rel=\"stylesheet\" href=\"recursive.css\" media=\"screen\">\n\
    </head>\n\
    <body class=\"blog-index\">\n\
        <header>\n\
            <h1 class=\"blog-title\">subnut's blog</h1>\n\
        </header>\n\
        <div id=\"wrapper\">\n\
            <table class=\"blog-index\">\n\
<!-- Index starts here -->\n\
";

static const char FINAL_TEXT[] = "\
<!-- Index ends here -->\n\
            </table>\n\
            <br>\n\
            <br>\n\
            %s\n\
        </div>\n\
    </body>\n\
</head>\n\
";

int
main(int argc, const char **argv)
{
	DIR *dir;
	FILE *outfile;
	struct dirent *dirent;
	char filenames[MAX_FILES + 1][FILENAME_MAX + 1];	// +1 because index starts at 0

	// Mark all records as empty
	for (int i=1; i <= MAX_FILES; i++)
		*filenames[i] = '\0';

	if (cd(DEST_DIR))
		return 1;

	outfile = fopen("index.html", "w");
	fprintf(outfile, INITIAL_TEXT, FAVICON);

	if ((dir = opendir(".")) == NULL)
	{
		fprintf(stderr, "index: opendir error");
		return 1;
	}
	while ((dirent = readdir(dir)) != NULL)
	{
		char *name = dirent->d_name;
		if (
				name[0] == '.'		// ie. ., .., and hidden files
				|| name[0] == '0'	// ie. 0-draft, 0-format, etc.
				|| memcmp(strrchr(name, '.'), ".html", 5)	// not *.html
				|| !memcmp(name, "index.html", 10)
		   )
			continue;

		char *p = memchr(name, '-', FILENAME_MAX);	// defined in stdio.h
		*p = '\0';

		int num = stoi(name);
		*p = '-';

		strcpy(filenames[num], name);
	}
	closedir(dir);


	FILE *file;

	/*
	 * Title.
	 * +7 for the leading "TITLE: "
	 * +2 for the trailing '\n' and '\0'
	 */
	char TITLE[MAX_TITLE_LENGTH + 9];

	/*
	 * Date published.
	 * 10 for the "YYYY/MM/DD"
	 * +9 for the leading "CREATED:"
	 * +1 for the trailing '\0'
	 */
	char DATE_CREATED[20];

	for (int i=1; *filenames[i]!='\0' && i<=MAX_FILES ; i++)
	{
		/* Skip if filename is invalid */
		if ((file = fopen(filenames[i], "r")) == NULL)
			continue;

		/* Remove first line */
        char first_line[6];
		fgets(first_line, 6, file);	// "<!--\n" +1 for '\0'

		/* Title */
		fgets(TITLE, MAX_TITLE_LENGTH, file);
		memmove(TITLE, TITLE + 6, MAX_TITLE_LENGTH - 6);	// Remove "TITLE:"
		while (*TITLE == ' ')
			memmove(TITLE, TITLE + 1, MAX_TITLE_LENGTH - 1);
		*(strrchr(TITLE, '\n')) = '\0';

		/* Date created */
		fgets(DATE_CREATED, 20, file);
		memmove(DATE_CREATED, DATE_CREATED + 9, 20 - 9);	// Remove "DATE_CREATED:"
		while (*DATE_CREATED == ' ')
			memmove(DATE_CREATED, TITLE + 1, 20 - 1);

		/* Done reading from file */
		fclose(file);


		char DATE_CREATED_str[15];
		char url[FILENAME_MAX*3 + 1];

		urlencode_s(filenames[i], url);
		date_to_text(DATE_CREATED, DATE_CREATED_str);

		/*
		fprintf(outfile,
				"<tr>\n"
				"    <td class=\"blog-index-name\">\n"
				"        <a href=\"%s\">%s</a>\n"
				"    </td>\n"
				"    <td class=\"blog-index-date\">\n"
				"        %s\n"
				"    </td>\n"
				"</tr>\n",
				url,
				TITLE,
				DATE_CREATED_str
			 );
		*/

		fprintf(outfile,
				"<tr>\n"
				"    <td class=\"blog-index-name\">\n"
				"        <a href=\"%s\">",
				url);
		fputs_escaped(TITLE, outfile);
		fprintf(outfile,                  "</a>\n"
				"    </td>\n"
				"    <td class=\"blog-index-date\">\n"
				"        %s\n"
				"    </td>\n"
				"</tr>\n",
				DATE_CREATED_str
			 );

#ifdef PRINT_FILENAMES
		printf("%i:\t%s\n", i, filenames[i]);
#endif /* PRINT_FILENAMES */

	}

	fprintf(outfile, FINAL_TEXT, FOOTER);
	fclose(outfile);

	return 0;
}

// vim:noet:ts=4:sts=0:sw=0:fdm=syntax:nowrap
