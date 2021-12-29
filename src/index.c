#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * _POSIX_C_SOURCE - strdup, getline
 *
 * dirent.h - opendir, readdir
 * stdio.h  - fopen, fclose
 * string.h - memcmp, memchr, strdup, strlen
 * stdlib.h - free, EXIT_{SUCCESS,FAILURE}
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

	struct {
		int maxindex;
		char **names;	// Array of strings
	} filenames;

	if ((filenames.names = calloc(1, sizeof(char*))) == NULL)
		return perror("calloc error"),
			   EXIT_FAILURE;

	filenames.maxindex = 0;
	filenames.names[0] = NULL;

	if (cd(DEST_DIR))
		return perror("cd error"),
			   EXIT_FAILURE;

	outfile = fopen("index.html", "w");
	fprintf(outfile, INITIAL_TEXT, FAVICON);

	if ((dir = opendir(".")) == NULL)
		return perror("opendir error:"),
			   EXIT_FAILURE;

	while ((dirent = readdir(dir)) != NULL)
	{
		char *name = dirent->d_name;
		if (name[0] == '.'		// ie. ., .., and hidden files
			|| name[0] == '0'	// ie. 0-draft, 0-format, etc.
			|| memcmp(strrchr(name, '.'), ".html", 5)	// not *.html
			|| !memcmp(name, "index.html", 10)
		) continue;

		char *name_allocd;
		if ((name_allocd = strdup(name)) == NULL)
			return perror("strdup error"),
				   EXIT_FAILURE;

		char *p;
		*(p = strchr(name_allocd, '-')) = '\0';
		int num = stoi(name_allocd);
		*p = '-';

		if (num > filenames.maxindex)
		{
			if ((filenames.names = realloc(filenames.names, (num+1) * sizeof(char*))) == NULL)
				return perror("realloc error"),
					   EXIT_FAILURE;
			for (int i = filenames.maxindex+1; i <= num; i++)
				filenames.names[i] = NULL;
			filenames.maxindex = num;
		}
		filenames.names[num] = name_allocd;
	}
	closedir(dir);


	/*
	 * Date published.
	 * 10 for the "YYYY/MM/DD"
	 * +9 for the leading "CREATED:"
	 * +1 for the trailing '\0'
	 */
	char DATE_CREATED[20];

	FILE *infile;	// input file
	char *Title;
	size_t _ = 0;
	for (int i=1; i <= filenames.maxindex; i++)
	{
		/* Skip if NULL */
		if (filenames.names[i] == NULL)
			continue;

		/* Open file for reading */
		if ((infile = fopen(filenames.names[i], "r")) == NULL)
			return perror("Failed to open infile"),
				   EXIT_FAILURE;

		/* Remove first line */
        char first_line[6];	// 6 because "<!--\n" has a trailing '\0'
		if (fgets(first_line, 6, infile) == NULL)
			return fprintf(stderr, "Error while reading from file %s\n", filenames.names[i]),
				   EXIT_FAILURE;

		/* Title */
		if (getline(&Title, &_, infile) == -1)
			return perror("getline error"),
				   EXIT_FAILURE;
		memmove(Title, Title + 6, strlen(Title) - 6);	// Remove "TITLE:"
		while (*Title == ' ')
			memmove(Title, Title + 1, strlen(Title) - 1);
		*(strrchr(Title, '\n')) = '\0';

		/* Date created */
		if (fgets(DATE_CREATED, 20, infile) == NULL)
			return fprintf(stderr, "Error while reading from file %s\n", filenames.names[i]),
				   EXIT_FAILURE;
		memmove(DATE_CREATED, DATE_CREATED + 9, 20 - 9);	// Remove "CREATED:"
		while (*DATE_CREATED == ' ')
			memmove(DATE_CREATED, DATE_CREATED + 1, 20 - 1);

		/* Done reading from file */
		fclose(infile);


		char DATE_CREATED_str[15];
		char url[FILENAME_MAX*3 + 1];

		date_to_text(DATE_CREATED, DATE_CREATED_str);
		urlencode_s(filenames.names[i], url, FILENAME_MAX*3+1);
		free(filenames.names[i]);
		filenames.names[i] = NULL;

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
				Title,
				DATE_CREATED_str
			 );
		*/

		fprintf(outfile,
				"<tr>\n"
				"    <td class=\"blog-index-name\">\n"
				"        <a href=\"%s\">",
				url);

		fputs_escaped_allow_charrefs(Title, outfile);
		free(Title);
		Title = NULL;

		fprintf(outfile,                  "</a>\n"
				"    </td>\n"
				"    <td class=\"blog-index-date\">\n"
				"        %s\n"
				"    </td>\n"
				"</tr>\n",
				DATE_CREATED_str
			 );
	}

	fprintf(outfile, FINAL_TEXT, FOOTER);
	fclose(outfile);

	return EXIT_SUCCESS;
}

// vim:noet:ts=4:sts=0:sw=0:fdm=syntax:nowrap
