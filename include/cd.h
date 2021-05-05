#ifndef CD_H
#define CD_H

#include <unistd.h>
#include <errno.h>

int
cd(const char *path, const char **argv)
{
    if (chdir(path))
    {
        switch (errno)
        {
            case ENOENT:
                fprintf(stderr, "%s: directory not found: %s\n", *argv, path);
                break;
            case ENOTDIR:
                fprintf(stderr, "%s: not a directory: %s\n", *argv, path);
                break;
            case EACCES:
                fprintf(stderr, "%s: permission denied: %s\n", *argv, path);
                break;
            default:
                fprintf(stderr, "%s: chdir error (errno %i): %s\n", *argv, errno, path);
                break;
        }
        return 1;
    }
    return 0;
}

#endif
// vim:et:ts=4:sts=0:sw=0:fdm=syntax
