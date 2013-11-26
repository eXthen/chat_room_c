#include "number.h"
#include <stdio.h>

int getNumber(unsigned char *buf)
{
    int i = 0;
    int number = 0;

	for (i; i < 4; ++i)
        number += ((int) buf[i]) << (8 * (3 - i));
    
    return number;
}

void convertNumber(unsigned char *buf, int number)
{
    int i = 0;

	for (i; i < 4; ++i)
	    buf[3 - i] = (number >> (8 * i)) & 0xFF;
}
