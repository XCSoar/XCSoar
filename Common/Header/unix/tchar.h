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

#ifndef TCHAR_H
#define TCHAR_H

#include <string.h>
#include <stdarg.h>

#ifdef UNICODE

#include <wchar.h>
#include <wctype.h>

typedef wchar_t TCHAR;
#define _stprintf wsprintf
#define _sntprintf wsnprintf
#define _ftprintf fwprintf
#define _vftprintf vfwprintf
#define _tcscpy wcscpy
#define tcslen wcslen
#define _tcscmp wcscmp
#define _tcslen wcslen
#define _tcsclen wcslen
#define _tcsncpy wcsncpy
#define _tcsstr wcsstr
#define _tcschr wcschr
#define _tcscat wcscat
#define _tcsncat wcsncat
#define TEXT(x) (L ## x)
#define _T(x) (L ## x)
#define _fgetts fgetws
#define _stscanf swscanf
#define _tcstok wcstok
//#define _totupper XXX
#define _itot _itow
#define _tcstol wcstol
#define _tcstod wcstod

#define _istalpha iswalpha
#define _istspace iswspace

#else /* !UNICODE */

#include <ctype.h>

typedef char TCHAR;
#define _stprintf sprintf
#define _sntprintf snprintf
#define _ftprintf fprintf
#define _vftprintf vfprintf
#define _tcscpy strcpy
#define tcslen strlen
#define _tcscmp strcmp
#define _tcslen strlen
#define _tcsclen strlen
#define _tcsncpy strncpy
#define _tcsstr strstr
#define _tcschr strchr
#define _tcscat strcat
#define _tcsncat strncat
#define TEXT(x) x
#define _T(x) x
#define _tfopen fopen
#define _fgetts fgets
#define _stscanf sscanf
#define _tcstok strtok
#define _totupper toupper
#define _itot itoa
#define _tcstol strtol
#define _tcstod strtod
#define _tcscspn strcspn

#define _istalpha isalpha
#define _istspace isspace

#endif /* UNICODE */

#define _tcsupr CharUpper

typedef TCHAR *LPTSTR;
typedef const TCHAR *LPCTSTR;

static inline LPTSTR
CharUpper(LPTSTR s)
{
  LPTSTR p;
  for (p = s; *p != 0; ++p)
#ifdef UNICODE
    *p = towupper(*p);
#else
    *p = toupper(*p);
#endif
  return s;
}

typedef wchar_t WCHAR;
typedef WCHAR *LPWSTR;
typedef const WCHAR *LPCWSTR;

/*
static inline void
wsprintf(LPWSTR buffer, LPCWSTR format, ...)
{
  va_list ap;
  va_start(ap, format);
  vswprintf(buffer, 65536, format, ap);
}
*/

#endif
