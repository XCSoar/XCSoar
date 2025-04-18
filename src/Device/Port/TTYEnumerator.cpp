// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TTYEnumerator.hpp"
#include "util/CharUtil.hxx"
#include "util/StringCompare.hxx"
#include "system/FileUtil.hpp"

#include <stdio.h>
#include <unistd.h>

[[gnu::pure]]
static bool
CheckTTYName(const char *name) noexcept
{
  /* filter "/dev/tty*" */
  if (const char *t = StringAfterPrefix(name, "tty"); t != nullptr) {
    if (*t == 0)
      /* ignore /dev/tty */
      return false;

    /* ignore pseudo tty slaves (found on Kobo, macOS and probably
       many others) */
    if (((*t >= 'a' && *t <= 'e') || (*t >= 'p' && *t <= 'z')) &&
        IsLowerHexDigit(t[1]) && t[2] == 0)
      return false;

    /* filter out "/dev/tty0", ... (valid integer after "tty") */
    if (IsDigitASCII(*t))
      return false;

    return true;
  } else if (StringStartsWith(name, "rfcomm"))
    return true;
  else
    return false;
}

const char *
TTYEnumerator::Next() noexcept
{
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    if (!CheckTTYName(ent->d_name))
      continue;

    if (snprintf(path, sizeof(path), "/dev/%s", ent->d_name) >= (int)sizeof(path))
      /* truncated - ignore */
      continue;

    if (File::IsCharDev(Path{path}) && access(path, R_OK|W_OK) == 0)
      return path;
  }

  return nullptr;
}
