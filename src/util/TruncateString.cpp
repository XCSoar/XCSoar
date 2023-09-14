// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TruncateString.hpp"
#include "StringAPI.hxx"
#include "UTF8.hpp"

#include <algorithm>

#include <cassert>

char *
CopyTruncateString(char *dest, size_t dest_size, const char *src)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);

  size_t src_length = StringLength(src);
  size_t copy = std::min(src_length, dest_size - 1);

  auto *p = std::copy_n(src, copy, dest);
  *p = _T('\0');
  return CropIncompleteUTF8(dest);
}

#ifdef _UNICODE

TCHAR *
CopyTruncateString(TCHAR *dest, size_t dest_size, const TCHAR *src)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);

  size_t src_length = StringLength(src);
  size_t copy = std::min(src_length, dest_size - 1);

  auto *p = std::copy_n(src, copy, dest);
  *p = _T('\0');
  return p;
}

#endif

TCHAR *
CopyTruncateString(TCHAR *dest, size_t dest_size,
                   const TCHAR *src, size_t truncate)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);

#ifdef _UNICODE
  size_t src_length = StringLength(src);
  size_t copy = std::min({src_length, truncate, dest_size - 1});

  auto *p = std::copy_n(src, copy, dest);
  *p = _T('\0');
  return p;
#else
  return CopyTruncateStringUTF8(dest, dest_size, src, truncate);
#endif
}
