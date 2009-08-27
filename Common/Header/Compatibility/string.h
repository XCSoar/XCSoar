/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include <tchar.h>
#include <ctype.h>

#ifndef HAVE_MSVCRT

/* WINE is more strict, and doesn't give is sprintf() / _stprintf() if
   we don't include stdio.h */
#include <stdio.h>

static inline char *
_strupr(char *p)
{
  char *q;

  for (q = p; *q != 0; ++q)
    *q = toupper(*q);

  return p;
}

#define _istalnum isalnum
#define towupper toupper
#define _strnicmp strncasecmp
#define _strdup strdup

#else /* !HAVE_MSVCRT */

#define _tcsclen(x) _tcslen(x)

#ifdef __cplusplus
extern "C" {
#endif

_CRTIMP int __cdecl     _wtoi (const wchar_t *);

#ifdef __cplusplus
}
#endif

#endif /* HAVE_MSVCRT */

#endif
