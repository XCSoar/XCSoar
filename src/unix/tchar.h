// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef LIBCXX
/* libc++ uses "_T" as template argument names; this conflicts with
   the macro "_T()" defined below.  To work around that problem,
   include all relevant libc++ headers before defining _T() */
#include <iterator>
#endif

typedef char TCHAR;

// Safe wrappers that use snprintf/vsnprintf instead of deprecated sprintf/vsprintf
// Note: These require the buffer size to be known at compile time or passed explicitly
// For compatibility with existing _stprintf calls, we use a macro that captures the buffer
#define _stprintf(buffer, ...) snprintf(buffer, sizeof(buffer), __VA_ARGS__)
#define _vstprintf(buffer, format, args) vsnprintf(buffer, sizeof(buffer), format, args)
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
