/***********************************************************************
**
**   utils.cpp
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Heiner Lamprecht
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include "utils.h"

#include <ctype.h>
#include <stdio.h>

char *utoa(unsigned value, char *digits, int base)
{
    const char *s = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (base == 0)
        base = 10;
    if (digits == NULL || base < 2 || base > 36)
        return NULL;
    if (value < (unsigned) base) {
        digits[0] = s[value];
        digits[1] = '\0';
    } else {
        char *p;
        for (p = utoa(value / ((unsigned)base), digits, base);
             *p;
             p++) {}
        utoa( value % ((unsigned)base), p, base);
    }
    return digits;
}

#ifndef HAVE_MSVCRT

char *strupr(char *str)
{
  char *string = str;

  if (str)
    {
      for ( ; *str; ++str)
          *str = toupper(*str);
    }

  return string;
}
#endif
