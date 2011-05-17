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

#ifndef TCHAR_H
#define TCHAR_H

#include <string.h>
#include <stdarg.h>

#ifdef UNICODE

#include <wchar.h>
#include <wctype.h>

typedef wchar_t TCHAR;
#define _stprintf wsprintf
#define _vstprintf vswprintf
#define _sntprintf wsnprintf
#define _vsntprintf wsnprintf
#define _tprintf wprintf
#define _ftprintf fwprintf
#define _vftprintf vfwprintf
#define _fputts fputws
#define _tcsdup wcsdup
#define _tcscpy wcscpy
#define tcslen wcslen
#define _tcscmp wcscmp
#define _tcsncmp wcscmp
#define _tcsicmp wcscasecmp
#define _tcsnicmp wcsncasecmp
#define _tcslen wcslen
#define _tcsclen wcslen
#define _tcsncpy wcsncpy
#define _tcsstr wcsstr
#define _tcschr wcschr
#define _tcsrchr wcsrchr
#define _tcspbrk wcspbrk
#define _tcscat wcscat
#define _tcsncat wcsncat
#define _T(x) (L ## x)

#define _TEOF EOF
#define _fgetts fgetws
#define _putts(t) putws((t), stdout)
#define _stscanf swscanf

#define _tremove _wremove
#define _tunlink _wunlink

#define _tcstok wcstok
//#define _totupper XXX
#define _itot _itow
#define _ttoi(x) wcstol((x), NULL, 10)
#define _tcstol wcstol
#define _tcstoul wcstoul
#define _tcstod wcstod

#define _istalpha iswalpha
#define _istalnum iswalnum
#define _istspace iswspace
#define _istdigit iswdigit

#else /* !UNICODE */

#include <ctype.h>

typedef char TCHAR;
#define _stprintf sprintf
#define _vstprintf vsprintf
#define _sntprintf snprintf
#define _vsntprintf vsnprintf
#define _tprintf printf
#define _ftprintf fprintf
#define _vftprintf vfprintf
#define _fputts fputs
#define _tcsdup strdup
#define _tcscpy strcpy
#define tcslen strlen
#define _tcscmp strcmp
#define _tcsncmp strncmp
#define _tcsicmp strcasecmp
#define _tcsnicmp strncasecmp
#define _tcslen strlen
#define _tcsclen strlen
#define _tcsncpy strncpy
#define _tcsstr strstr
#define _tcschr strchr
#define _tcsrchr strrchr
#define _tcspbrk strpbrk
#define _tcscat strcat
#define _tcsncat strncat
#define _T(x) x
#define _TDIR DIR
#define _topendir opendir
#define _tclosedir closedir
#define _treaddir readdir
#define _tdirent dirent
#define _tstat stat
#define _tfopen fopen
#define _TEOF EOF
#define _fgetts fgets
#define _putts puts
#define _stscanf sscanf

#define _tremove remove
#define _tunlink unlink

#define _tcstok strtok
#define _totupper toupper
#define _itot itoa
#define _ttoi atoi
#define _tcstol strtol
#define _tcstoul strtoul
#define _tcstod strtod
#define _tcscspn strcspn

#define _istalpha isalpha
#define _istalnum isalnum
#define _istdigit isdigit
#define _istspace isspace

#endif /* UNICODE */

#define _tcsupr CharUpper

static inline TCHAR *
CharUpper(TCHAR *s)
{
  TCHAR *p;
  for (p = s; *p != 0; ++p)
#ifdef UNICODE
    *p = towupper(*p);
#else
    *p = (TCHAR)toupper(*p);
#endif
  return s;
}

#endif
