// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/PathName.hpp"
#include "util/StringAPI.hxx"

[[gnu::pure]]
static const char *
LastSeparator(const char *path)
{
  const auto *p = StringFindLast(path, _T('/'));
#ifdef _WIN32
  const auto *backslash = StringFindLast(path, _T('\\'));
  if (p == nullptr || backslash > p)
    p = backslash;
#endif
  return p;
}

[[gnu::pure]]
static char *
LastSeparator(char *path)
{
  return const_cast<char *>(LastSeparator((const char *)path));
}

void
ReplaceBaseName(char *path, const char *new_base)
{
  char *q = LastSeparator(path);
  if (q != nullptr)
    ++q;
  else
    q = path;
  strcpy(q, new_base);
}
