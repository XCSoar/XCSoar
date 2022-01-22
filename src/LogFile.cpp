/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "Asset.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "Formatter/TimeFormatter.hpp"
#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "io/UniqueFileDescriptor.hxx"
#include "util/Exception.hxx"

#include <cwchar>

#include <stdio.h>
#include <stdarg.h>
#include <windef.h> // for MAX_PATH

#ifdef ANDROID
#include <android/log.h>
#include <fcntl.h>
#endif

static FileOutputStream
OpenLog()
{
  static bool initialised = false;
  static AllocatedPath path = nullptr;

  if (!initialised) {
    initialised = true;

    /* delete the obsolete log file */
    File::Delete(LocalPath(_T("xcsoar-startup.log")));

    path = LocalPath(_T("xcsoar.log"));

    File::Replace(path, LocalPath(_T("xcsoar-old.log")));

#ifdef ANDROID
    /* redirect stdout/stderr to xcsoar-startup.log on Android so we
       get debug logs from libraries and output from child processes
       there */
    UniqueFileDescriptor fd;
    if (fd.Open(path.c_str(), O_APPEND|O_CREAT|O_WRONLY, 0666)) {
      fd.CheckDuplicate(FileDescriptor(STDOUT_FILENO));
      fd.CheckDuplicate(FileDescriptor(STDERR_FILENO));
    }
#endif
  }

  return FileOutputStream{path, FileOutputStream::Mode::APPEND_OR_CREATE};
}

static void
LogString(const char *p) noexcept
{
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "XCSoar", "%s", p);
#elif defined(HAVE_POSIX) && !defined(NDEBUG)
  fprintf(stderr, "%s\n", p);
#endif

  try {
    auto fos = OpenLog();
    BufferedOutputStream bos{fos};

    bos.Write('[');

    {
      char time_buffer[32];
      FormatISO8601(time_buffer, BrokenDateTime::NowUTC());
      bos.Write(time_buffer);
    }

    bos.Write("] ");
    bos.Write(p);
    bos.NewLine();

    bos.Flush();
    fos.Commit();
  } catch (...) {
  }
}

void
LogFormat(const char *fmt, ...) noexcept
{
  char buf[MAX_PATH];
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  va_end(ap);

  LogString(buf);
}

#ifdef _UNICODE

static void
LogString(const wchar_t *p) noexcept
{
  try {
    auto fos = OpenLog();
    BufferedOutputStream bos{fos};

    bos.Write('[');

    {
      char time_buffer[32];
      FormatISO8601(time_buffer, BrokenDateTime::NowUTC());
      bos.Write(time_buffer);
    }

    bos.Write("] ");
    bos.Write(p);
    bos.NewLine();

    bos.Flush();
    fos.Commit();
  } catch (...) {
  }
}

void
LogFormat(const wchar_t *Str, ...) noexcept
{
  wchar_t buf[MAX_PATH];
  va_list ap;

  va_start(ap, Str);
  std::vswprintf(buf, std::size(buf), Str, ap);
  va_end(ap);

  LogString(buf);
}

#endif

void
LogError(std::exception_ptr e) noexcept
{
  LogString(GetFullMessage(e).c_str());
}

void
LogError(std::exception_ptr e, const char *msg) noexcept
{
  LogFormat("%s: %s", msg, GetFullMessage(e).c_str());
}
