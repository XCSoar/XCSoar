/***********************************************************************
**
**   vlapihlp.h
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef VLAPIHLP_H
#define VLAPIHLP_H


#include "vlapityp.h"

// some generally needed helper-functions


char *igc_filter(char *);
char *strtrim(char *);
char *wordtoserno(word);
long pressure2altitude(word);


#endif
