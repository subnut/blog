#ifndef CD_H
#define CD_H

#include <unistd.h>
#include <errno.h>

int cd(const char *path)
{
    if (chdir(path))
    {
        switch (errno)
        {
            case ENOENT:
                fprintf(stderr, "cd: directory not found: %s\n", path);
                break;
            case ENOTDIR:
                fprintf(stderr, "cd: not a directory: %s\n", path);
                break;
            case EACCES:
                fprintf(stderr, "cd: permission denied: %s\n", path);
                break;
            default:
                fprintf(stderr, "cd: chdir error: %s\n", path);
                break;
        }
        return 1;
    }
    return 0;
}

#endif
// vim:et:ts=4:sts=0:sw=0:fdm=syntax
