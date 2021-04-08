#include <string.h>
#include "stoi.h"

char *date_to_text(const char *date_str, const int long_month)
{
    char *output;
    char final_str[20];
    output = final_str;

    char working_copy[10];
    memmove(working_copy, date_str, 10);

    char *year  = working_copy;
    char *month = working_copy + 4;
    char *date  = working_copy + 7;

    *month++  = '\0';
    *date++   = '\0';
    *(date+2) = '\0';


    /* Date */
    switch (stoi(date))
    {
        case 1:
            memmove(output, "1st", 3);
            output += 3;
            break;
        case 2:
            memmove(output, "2nd", 3);
            output += 3;
            break;
        case 3:
            memmove(output, "3rd", 3);
            output += 3;
            break;

        case 4: output[0] = '4';
        case 5: output[0] = '5';
        case 6: output[0] = '6';
        case 7: output[0] = '7';
        case 8: output[0] = '8';
        case 9: output[0] = '9';
                output++;
                memmove(output, "th", 2);
                output += 3;
                break;

        default:
            output[0] = date[0];
            output[1] = date[1];
            output[2] = 't';
            output[3] = 'h';
            output += 4;
            break;
    }
    *output++ = ' ';


    /* Month */
    if (long_month)
        switch (stoi(month))
    {
        case 1:
            memmove(output, "January", 7);
            output += 7;
            break;
        case 2:
            memmove(output, "February", 8);
            output += 8;
            break;
        case 3:
            memmove(output, "March", 5);
            output += 5;
            break;
        case 4:
            memmove(output, "April", 5);
            output += 5;
            break;
        case 5:
            memmove(output, "May", 3);
            output += 3;
            break;
        case 6:
            memmove(output, "June", 4);
            output += 4;
            break;
        case 7:
            memmove(output, "July", 4);
            output += 4;
            break;
        case 8:
            memmove(output, "August", 6);
            output += 6;
            break;
        case 9:
            memmove(output, "September", 9);
            output += 9;
            break;
        case 10:
            memmove(output, "October", 7);
            output += 7;
            break;
        case 11:
            memmove(output, "November", 8);
            output += 8;
            break;
        case 12:
            memmove(output, "December", 8);
            output += 8;
            break;
    }
    else
        switch (stoi(month))
        {
            case 1:
                memmove(output, "Jan", 3);
                output += 3;
                break;
            case 2:
                memmove(output, "Feb", 3);
                output += 3;
                break;
            case 3:
                memmove(output, "Mar", 3);
                output += 3;
                break;
            case 4:
                memmove(output, "Apr", 3);
                output += 3;
                break;
            case 5:
                memmove(output, "May", 3);
                output += 3;
                break;
            case 6:
                memmove(output, "June", 4);
                output += 4;
                break;
            case 7:
                memmove(output, "July", 4);
                output += 4;
                break;
            case 8:
                memmove(output, "Aug", 3);
                output += 3;
                break;
            case 9:
                memmove(output, "Sep", 3);
                output += 3;
                break;
            case 10:
                memmove(output, "Oct", 3);
                output += 3;
                break;
            case 11:
                memmove(output, "Nov", 3);
                output += 3;
                break;
            case 12:
                memmove(output, "Dec", 3);
                output += 3;
                break;
        }
    *output++ = ' ';


    /* Year */
    memmove(output, year, 4);
    output += 4;
    *output++ = '\0';


    return final_str;
}

// vim:et:ts=4:sts=0:sw=0:fdm=syntax
