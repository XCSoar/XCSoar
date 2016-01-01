/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "LogError.hpp"
#include "LogFile.hpp"
#include "Util/Macros.hpp"
#include "Util/StringUtil.hpp"

#include <stdio.h>
#include <stdarg.h>

#ifdef WIN32

#include <windows.h>

void
LogLastError(const TCHAR *fmt, ...)
{
  const DWORD error = GetLastError();

  TCHAR buffer[1024];
  va_list ap;
  va_start(ap, fmt);
  _vsntprintf(buffer, ARRAY_SIZE(buffer), fmt, ap);
  va_end(ap);

  TCHAR msg[256];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, error, 0, msg, ARRAY_SIZE(msg), nullptr);
  StripRight(msg);

  LogFormat(_T("%s: %s"), buffer, msg);
}

#endif

#ifdef HAVE_POSIX

#include <string.h>
#include <errno.h>

void
LogErrno(const TCHAR *fmt, ...)
{
  const int error = errno;

  TCHAR buffer[1024];
  va_list ap;
  va_start(ap, fmt);
  _vsntprintf(buffer, ARRAY_SIZE(buffer), fmt, ap);
  va_end(ap);

  LogFormat(_T("%s: %s"), buffer, strerror(error));
}

#endif
