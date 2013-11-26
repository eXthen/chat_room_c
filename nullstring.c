#include <string.h>

#include "nullstring.h"

void nullStrcat(unsigned char *to, unsigned char *from, int offset)
{
    strcat(to + offset, from);
}

int nullStrlen(unsigned char *str, int offset)
{
    return (offset + strlen(str + offset));
}

void nullStrcpyFrom(unsigned char *to, unsigned char *from, int offset)
{
    strcpy(to, from + offset);
}

void nullStrcpyTo(unsigned char *to, unsigned char *from, int offset)
{
    strcpy(to + offset, from);
}

void nullStrcpy(unsigned char *to, unsigned char *from)
{
    int i;

    for (i = 0; i < 4; ++i)
        to[i] = from[i];

    strcpy(to + 4, from + 4);
}

void nullInsertType(unsigned char *str, char c)
{
    unsigned char T;

    switch (c) {
	    case 'A':
	    case 'a':
	        T = 0xFA;
	        break;
	    case 'B':
	    case 'b':
	        T = 0xFB;
	        break;
	    case 'C':
	    case 'c':
	        T = 0xFC;
	        break;
	    case 'D':
	    case 'd':
	        T = 0xFD;
	        break;
		default:
	        break;
    }

    str[4] = '|';
    str[5] = T;
    str[6] = '|';
    str[7] = '\0';
}


