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
**   $Id$
**
***********************************************************************/

char *utoa(unsigned value, char *digits, int base);

#ifdef HAVE_MSVCRT

#include <stdlib.h>

#else

char *strupr(char *str);

#endif


