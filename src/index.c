#define _POSIX_C_SOURCE 200809L

#include "constants.h"
#include "include/date_to_text.h"
#include "include/escape.h"
#include "include/free.h"
#include "include/perror.h"
#include "include/urlencode.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * _POSIX_C_SOURCE	- strdup, getline
 * dirent.h			- opendir, readdir
 * stdio.h			- fopen, fclose
 * string.h			- memcmp, memchr, strdup, strlen
 * stdlib.h			- EXIT_{SUCCESS,FAILURE}
 * unistd.h			- chdir
 */

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

	if (chdir(DEST_DIR))
		return perror("chdir error"),
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

		int index = atoi(name);	// Will ignore the '-' in the name automatically.
		if (index > filenames.maxindex)
		{
			if ((filenames.names = realloc(filenames.names, (index+1) * sizeof(char*))) == NULL)
				return perror("realloc error"),
					   EXIT_FAILURE;
			for (int i = filenames.maxindex+1; i <= index; i++)
				filenames.names[i] = NULL;
			filenames.maxindex = index;
		}
		filenames.names[index] = name_allocd;
	}
	closedir(dir);

	size_t _;
	char *TITLE;
	char *CREATED;
	FILE *file;
	for (int i=1; i <= filenames.maxindex; i++)
	{
		/* Skip if NULL */
		if (filenames.names[i] == NULL)
			continue;

		/* Open file for reading */
		if ((file = fopen(filenames.names[i], "r")) == NULL)
			return perror("Failed to open file"),
				   EXIT_FAILURE;

		/* Remove first line */
        char first_line[6];	// 6 because "<!--\n" has a trailing '\0'
		if (fgets(first_line, 6, file) == NULL)
			return fprintf(stderr, "Error while reading from file %s\n", filenames.names[i]),
				   EXIT_FAILURE;

		/* TITLE */
		_ = 0; TITLE = NULL;
		if (getline(&TITLE, &_, file) == -1)
			return perror("getline error"),
				   EXIT_FAILURE;
		memmove(TITLE, TITLE + 6, strlen(TITLE) - 6);	// Remove "TITLE:"
		while (*TITLE == ' ')
			memmove(TITLE, TITLE + 1, strlen(TITLE) - 1);
		*(strrchr(TITLE, '\n')) = '\0';

		/* Date created */
		_ = 0; CREATED = NULL;
		if (getline(&CREATED, &_, file) == -1)
			return perror("getline error"),
				   EXIT_FAILURE;
		memmove(CREATED, CREATED + 9, strlen(CREATED) - 9);	// Remove "CREATED:"
		while (*CREATED == ' ')
			memmove(CREATED, CREATED + 1, strlen(CREATED) - 1);
		*(strrchr(CREATED, '\n')) = '\0';

		/* Done reading from file */
		fclose(file);

		/* Make filename from URL */
		char *url;
		url = malloc(strlen(filenames.names[i])*3 + 1); // +1 for trailing '\0'
		urlencode_s(filenames.names[i], url, strlen(filenames.names[i])*3+1);
		free(filenames.names[i]);

		fprintf(outfile,
				"<tr>\n"
				"    <td class=\"blog-index-name\">\n"
				"        <a href=\"%s\">",
				url);
		fputs_escaped_allow_charrefs(TITLE, outfile);
		fprintf(outfile,                  "</a>\n"
				"    </td>\n"
				"    <td class=\"blog-index-date\">\n"
				"        %s\n"
				"    </td>\n"
				"</tr>\n",
				date_to_text(CREATED)
			   );

		free(url);
		free(TITLE);
		free(CREATED);
	}

	fprintf(outfile, FINAL_TEXT, FOOTER);
	fclose(outfile);
	return EXIT_SUCCESS;
}

/* vim: set noet nowrap fdm=syntax: */
