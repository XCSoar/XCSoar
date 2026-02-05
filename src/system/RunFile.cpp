// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RunFile.hpp"

#ifdef ANDROID
// replaced by NativeView::OpenWaypointFile()
#elif defined(HAVE_POSIX) && !defined(_WIN32) && !defined(KOBO)

#include "Process.hpp"

bool
RunFile(const char *path) noexcept
{
#if defined(__APPLE__)
  return Start("/usr/bin/open", path);
#else
  return Start("/usr/bin/xdg-open", path);
#endif
}

#endif
