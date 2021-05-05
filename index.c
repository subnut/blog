#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

/*
 * ctype.h  - isalnum
 * stdio.h  - fopen, fclose
 * string.h - memcmp, memchr
 * dirent.h - opendir, readdir
 */

#include "include/cd.h"
#include "include/date.h"
#include "include/stoi.h"
#include "constants.h"

#define MAX_FILES        100
#define MAX_TITLE_LENGTH 150
#define cd(x) \
        cd(x, argv)

const char INITIAL_TEXT[] = "\
<html>\n\
    <head>\n\
        <meta charset=\"utf-8\"/>\n\
        <title>subnut's blog</title>\n\
        %s\n\
        <link rel=\"stylesheet\" href=\"style.css\">\n\
        <link rel=\"stylesheet\" href=\"recursive.css\">\n\
    </head>\n\
    <body class=\"blog-index\">\n\
        <h1 class=\"title\">subnut's blog</h1>\n\
        <table class=\"blog-index\">\n\
<!-- Index starts here -->\n\
";

const char FINAL_TEXT[] = "\
<!-- Index ends here -->\n\
        </table>\n\
        <br>\n\
        <br>\n\
        %s\n\
    </body>\n\
</head>\n\
";

char *
urlencode_c(const char c, char *str_size_4)
{
	/* unreserved */
	if (isalnum(c))
	{
		sprintf(str_size_4, "%c", c);
		return str_size_4;
	}
	switch (c)
	{
		case '-':
		case '.':
		case '_':
		case '~':
			sprintf(str_size_4, "%c", c);
			return str_size_4;
		default:
			break;
	}

	/* reserved */
	sprintf(str_size_4, "%%%0X", c);
	return str_size_4;
}

char *
urlencode_s(const char *s, char *storage)
{
	char str[4];
	char *pointer;
	pointer = storage;
	for (int i=0; s[i]!='\0' && i<=FILENAME_MAX*4; i++)
	{
		urlencode_c(s[i], str);
		memmove(pointer, str, strlen(str));
		pointer += strlen(str);
	}
	*pointer = '\0';
	return storage;
}

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

	char TITLE[MAX_TITLE_LENGTH];
	char DATE_CREATED[11]; // "YYYY/MM/DD" +1 for '\0'

	for (int i=1; *filenames[i]!='\0' && i<=MAX_FILES ; i++)
	{
		/* Skip if filename is invalid */
		if ((file = fopen(filenames[i], "r")) == NULL)
			continue;

		/* Remove first line */
        char *first_line = "<!--\n";
		fgets(first_line, 6, file);	// "<!--\n" +1 for '\0'

		/* Title */
		fgets(TITLE, MAX_TITLE_LENGTH, file);
		memmove(TITLE, TITLE + 6, MAX_TITLE_LENGTH - 6);	// Remove "TITLE:"
		while (*TITLE == ' ')
			memmove(TITLE, TITLE + 1, MAX_TITLE_LENGTH - 1);
		*(strrchr(TITLE, '\n')) = '\0';

		/* Date created */
		fgets(DATE_CREATED, 11, file);
		memmove(DATE_CREATED, DATE_CREATED + 9, 11 - 9);	// Remove "DATE_CREATED:"
		while (*DATE_CREATED == ' ')
			memmove(DATE_CREATED, TITLE + 1, 11 - 1);

		/* Done reading from file */
		fclose(file);


		char DATE_CREATED_str[15];
		char url[FILENAME_MAX*3 + 1];

		urlencode_s(filenames[i], url);
		date_to_text(DATE_CREATED, DATE_CREATED_str);

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

#ifdef PRINT_FILENAMES
		printf("%i:\t%s\n", i, filenames[i]);
#endif /* PRINT_FILENAMES */

	}

	fprintf(outfile, FINAL_TEXT, FOOTER);
	fclose(outfile);

	return 0;
}

// vim:noet:ts=4:sts=0:sw=0:fdm=syntax:nowrap
