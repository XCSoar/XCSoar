/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_COMPATIBILITY_STRING_H
#define XCSOAR_COMPATIBILITY_STRING_H

#ifndef HAVE_MSVCRT

#include <ctype.h>

static inline char *
_strupr(char *p)
{
  char *q;

  for (q = p; *q != 0; ++q)
    *q = (char)toupper(*q);

  return p;
}

#if !defined(_istalnum)
  #define _istalnum isalnum
#endif

#define _strnicmp strncasecmp
#define _strdup strdup

#else /* !HAVE_MSVCRT */

#ifdef _WIN32_WCE

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

_CRTIMP int __cdecl     _wtoi (const wchar_t *);

#ifdef __cplusplus
}
#endif

#endif /* _WIN32_WCE */

#endif /* HAVE_MSVCRT */

#endif
