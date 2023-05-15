// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogFile.hpp"
#include "util/Exception.hxx"

#include <exception>
#include <cstdarg>
#include <cstdio>

void
LogString(std::string_view s) noexcept
{
  fprintf(stderr, "%.*s\n",
          int(s.size()), s.data());
}

void
LogFormat(const char *fmt, ...) noexcept
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
}

#ifdef _UNICODE

void
LogFormat(const wchar_t *fmt, ...) noexcept
{
  va_list ap;

  va_start(ap, fmt);
  vfwprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
}

#endif

void
LogError(std::exception_ptr e) noexcept
{
  LogFormat("%s", GetFullMessage(e).c_str());
}

void
LogError(std::exception_ptr e, const char *msg) noexcept
{
  LogFormat("%s: %s", msg, GetFullMessage(e).c_str());
}
