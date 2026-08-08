// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Path.hpp"

#include <stdio.h>

#ifdef _WIN32
#include "UTF8Win32.hpp"
#endif

/**
 * fopen() with a UTF-8 #Path.  On Windows this uses _wfopen() so
 * non-ASCII paths work; elsewhere it is a plain fopen().
 *
 * Prefer this (or CreateFileW + UTF8ToWide) over CRT fopen()/_open()
 * whenever the path may contain non-ASCII characters.
 */
[[nodiscard]]
inline FILE *
OpenPathFile(Path path, const char *mode) noexcept
{
  if (mode == nullptr || *mode == '\0')
    return nullptr;

#ifdef _WIN32
  const std::wstring wpath = UTF8ToWide(path.c_str());
  const std::wstring wmode = UTF8ToWide(mode);
  if (wpath.empty() || wmode.empty())
    return nullptr;

  return _wfopen(wpath.c_str(), wmode.c_str());
#else
  return fopen(path.c_str(), mode);
#endif
}
