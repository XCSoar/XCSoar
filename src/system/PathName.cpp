// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/PathName.hpp"
#include "util/StringAPI.hxx"

[[gnu::pure]]
static const TCHAR *
LastSeparator(const TCHAR *path)
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
static TCHAR *
LastSeparator(TCHAR *path)
{
  return const_cast<TCHAR *>(LastSeparator((const TCHAR *)path));
}

void
ReplaceBaseName(TCHAR *path, const TCHAR *new_base)
{
  TCHAR *q = LastSeparator(path);
  if (q != nullptr)
    ++q;
  else
    q = path;
  _tcscpy(q, new_base);
}
