// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

#include <fmt/format.h>

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

void
LogString(std::string_view s) noexcept
{
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "XCSoar", "%.*s",
                      int(s.size()), s.data());
#elif defined(HAVE_POSIX) && !defined(NDEBUG)
  fprintf(stderr, "%.*s\n",
          int(s.size()), s.data());
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
    bos.Write(s);
    bos.NewLine();

    bos.Flush();
    fos.Commit();
  } catch (...) {
  }
}

void
LogVFmt(fmt::string_view format_str, fmt::format_args args) noexcept
{
	fmt::memory_buffer buffer;
#if FMT_VERSION >= 80000
	fmt::vformat_to(std::back_inserter(buffer), format_str, args);
#else
	fmt::vformat_to(buffer, format_str, args);
#endif
	LogString({buffer.data(), buffer.size()});
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
LogString(std::wstring_view s) noexcept
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
    bos.Write(s);
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
  LogString(GetFullMessage(e));
}

void
LogError(std::exception_ptr e, const char *msg) noexcept
{
  LogFormat("%s: %s", msg, GetFullMessage(e).c_str());
}
