#ifndef STOI_H
#define STOI_H

int ctoi(int c)
{
    switch (c)
    {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default:  return c;
    }
}
int stoi(const char *s)
{
    int retval = 0;
    for (int i = 0; s[i] != '\0'; i++)
    {
        retval *= 10;
        retval += ctoi(s[i]);
    }
    return retval;
}

#endif
// vim:et:ts=4:sts=0:sw=0:fdm=syntax
