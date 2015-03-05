/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifdef LIBCXX
/* libc++ uses "_T" as template argument names; this conflicts with
   the macro "_T()" defined below.  To work around that problem,
   include all relevant libc++ headers before defining _T() */
#include <iterator>
#endif

typedef char TCHAR;
#define _stprintf sprintf
#define _vstprintf vsprintf
#define _vsntprintf vsnprintf
#define _tprintf printf
#define _ftprintf fprintf
#define _vftprintf vfprintf
#define _fputts fputs
#define _tcsdup strdup
#define _tcscpy strcpy
#define _tcscmp strcmp
#define _tcslen strlen
#define _tcsclen strlen
#define _tcsstr strstr
#define _tcspbrk strpbrk
#define _tcscat strcat
#define _T(x) x
#define _topen open
#define _tfopen fopen
#define _TEOF EOF
#define _putts puts
#define _stscanf sscanf

#define _tcstol strtol
#define _tcstod strtod

#endif
