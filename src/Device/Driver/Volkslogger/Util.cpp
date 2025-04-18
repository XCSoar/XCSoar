// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Util.hpp"
#include "util/CharUtil.hxx"

#include <algorithm>

#include <cassert>
#include <string.h>

void
copy_padded(char *dest, size_t size, const char *src)
{
  assert(dest != nullptr);
  assert(size > 0);
  assert(src != nullptr);

  size_t src_length = strlen(src);
  if (src_length > size)
    src_length = size;

  memcpy(dest, src, src_length);
  memset(dest + src_length, ' ', size - src_length);
}

static char *
CopyUpper(char *dest, const char *src, const char *end)
{
  assert(dest != nullptr);
  assert(src != nullptr);
  assert(end >= src);

  while (src < end)
    *dest++ = ToUpperASCII(*src++);
  return dest;
}

void
CopyTerminatedUpper(char *dest, const char *src, size_t size)
{
  assert(dest != nullptr);
  assert(src != nullptr);
  assert(size > 0);

  const char *end = std::find(src, src + size, '\0');
  dest = CopyUpper(dest, src, end);
  *dest = '\0';
}

void
CopyPaddedUpper(char *dest, size_t size, const char *src)
{
  assert(dest != nullptr);
  assert(size > 0);
  assert(src != nullptr);

  size_t src_length = strlen(src);
  if (src_length > size)
    src_length = size;

  dest = CopyUpper(dest, src, src + src_length);
  memset(dest, ' ', size - src_length);
}
