#if 0

This file contains code to help me debug issues by printing the values of
variables/structs in a function

mod/htmlize.c
Used to print the lines inside the 'lines' struct.
=================================================
{
	for (int i = HISTORY-1; i >= 0; i--) {
		printf("HISTORY   %i:\t", i);
		if (data->lines->history[i] == NULL) puts("NULL");
		else {
			printf("%p: ", (void*)data->lines->history[i]);
			putchar('"');
			for (int j = 0; data->lines->history[i][j] != '\0'; j++)
				if (isgraph(data->lines->history[i][j]) || data->lines->history[i][j] == ' ')
					putchar(data->lines->history[i][j]);
				else
					printf("\\x%x", data->lines->history[i][j]);
			putchar('"');
			putchar('\n');
		}
	}
	for (int i = 0; i < READAHEAD; i++) {
		printf("READAHEAD %i:\t", i);
		if (data->lines->readahead[i] == NULL) puts("NULL");
		else {
			printf("%p: ", (void*)data->lines->readahead[i]);
			putchar('"');
			for (int j = 0; data->lines->readahead[i][j] != '\0'; j++)
				if (isgraph(data->lines->readahead[i][j]) || data->lines->readahead[i][j] == ' ')
					putchar(data->lines->readahead[i][j]);
				else
					printf("\\x%x", data->lines->readahead[i][j]);
			putchar('"');
			putchar('\n');
		}
	}
}

#endif
