/***********************************************************************
**
**   utils.h
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
**   $Id: utils.h,v 1.2 2008/02/14 14:22:48 samgi Exp $
**
***********************************************************************/

#if defined(__BORLANDC__)

#include <stdlib.h>

#else

char *utoa(unsigned value, char *digits, int base);

char *itoa(int value, char *digits, int base);

char *ltoa(long value, char *digits, int base);

char *strupr(char *str);

#endif


