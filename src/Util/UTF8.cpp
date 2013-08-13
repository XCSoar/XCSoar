/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Util/UTF8.hpp"
#include "Util/CharUtil.hpp"

#include <algorithm>

#include <assert.h>

/**
 * Is this a leading byte that is followed by 1 continuation byte?
 */
static constexpr bool
IsLeading1(unsigned char ch)
{
  return (ch & 0xe0) == 0xc0;
}

static constexpr unsigned char
MakeLeading1(unsigned char value)
{
  return 0xc0 | value;
}

/**
 * Is this a leading byte that is followed by 2 continuation byte?
 */
static constexpr bool
IsLeading2(unsigned char ch)
{
  return (ch & 0xf0) == 0xe0;
}

/**
 * Is this a leading byte that is followed by 3 continuation byte?
 */
static constexpr bool
IsLeading3(unsigned char ch)
{
  return (ch & 0xf8) == 0xf0;
}

/**
 * Is this a leading byte that is followed by 4 continuation byte?
 */
static constexpr bool
IsLeading4(unsigned char ch)
{
  return (ch & 0xfc) == 0xf8;
}

/**
 * Is this a leading byte that is followed by 5 continuation byte?
 */
static constexpr bool
IsLeading5(unsigned char ch)
{
  return (ch & 0xfe) == 0xfc;
}

static constexpr bool
IsContinuation(unsigned char ch)
{
  return (ch & 0xc0) == 0x80;
}

/**
 * Generate a continuation byte of the low 6 bit.
 */
static constexpr unsigned char
MakeContinuation(unsigned char value)
{
  return 0x80 | (value & 0x3f);
}

bool
ValidateUTF8(const char *p)
{
  for (; *p != 0; ++p) {
    unsigned char ch = *p;
    if (IsASCII(ch))
      continue;

    if (IsContinuation(ch))
      /* continuation without a prefix */
      return false;

    if (IsLeading1(ch)) {
      /* 1 continuation */
      if (!IsContinuation(*++p))
        return false;
    } else if (IsLeading2(ch)) {
      /* 2 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p))
        return false;
    } else if (IsLeading3(ch)) {
      /* 3 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p))
        return false;
    } else if (IsLeading4(ch)) {
      /* 4 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p) || !IsContinuation(*++p))
        return false;
    } else if (IsLeading5(ch)) {
      /* 5 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p))
        return false;
    } else
      return false;
  }

  return true;
}

static const char *
FindNonASCIIOrZero(const char *p)
{
  while (*p != 0 && IsASCII(*p))
    ++p;
  return p;
}

char *
Latin1ToUTF8(unsigned char ch, char *buffer)
{
  if (IsASCII(ch)) {
    *buffer++ = ch;
  } else {
    *buffer++ = MakeLeading1(ch >> 6);
    *buffer++ = MakeContinuation(ch);
  }

  return buffer;
}

const char *
Latin1ToUTF8(const char *gcc_restrict src, char *gcc_restrict buffer,
             size_t buffer_size)
{
  const char *p = FindNonASCIIOrZero(src);
  if (*p == 0)
    /* everything is plain ASCII, we don't need to convert anything */
    return src;

  if ((size_t)(p - src) >= buffer_size)
    /* buffer too small */
    return nullptr;

  const char *const end = buffer + buffer_size;
  char *q = std::copy(src, p, buffer);

  while (*p != 0) {
    unsigned char ch = *p++;

    if (IsASCII(ch)) {
      *q++ = ch;

      if (q >= end)
        /* buffer too small */
        return nullptr;
    } else {
      if (q + 2 >= end)
        /* buffer too small */
        return nullptr;

      *q++ = MakeLeading1(ch >> 6);
      *q++ = MakeContinuation(ch);
    }
  }

  *q = 0;
  return buffer;
}

size_t
LengthUTF8(const char *p)
{
  /* this is a very naive implementation: it does not do any
     verification, it just counts the bytes that are not a UTF-8
     continuation  */

  size_t n = 0;
  for (; *p != 0; ++p)
    if (!IsContinuation(*p))
      ++n;
  return n;
}

/**
 * Find the null terminator.
 */
gcc_pure
static char *
FindTerminator(char *p)
{
  assert(p != nullptr);

  while (*p != 0)
    ++p;

  return p;
}

/**
 * Find the leading byte for the given continuation byte.
 */
gcc_pure
static char *
FindLeading(gcc_unused char *const begin, char *i)
{
  assert(i > begin);
  assert(IsContinuation(*i));

  while (IsContinuation(*--i)) {
    assert(i > begin);
  }

  return i;
}

void
CropIncompleteUTF8(char *const p)
{
  assert(p != nullptr);

  char *const end = FindTerminator(p);
  if (end == p)
    return;

  char *const last = end - 1;
  if (!IsContinuation(*last)) {
    if (!IsASCII(*last))
      *last = 0;

    assert(ValidateUTF8(p));
    return;
  }

  char *const leading = FindLeading(p, last);
  const size_t n_continuations = last - leading;
  assert(n_continuations > 0);

  const unsigned char ch = *leading;

  unsigned expected_continuations;
  if (IsLeading1(ch))
    expected_continuations = 1;
  else if (IsLeading2(ch))
    expected_continuations = 2;
  else if (IsLeading3(ch))
    expected_continuations = 3;
  else if (IsLeading4(ch))
    expected_continuations = 4;
  else if (IsLeading5(ch))
    expected_continuations = 5;
  else {
    assert(n_continuations == 0);
    gcc_unreachable();
  }

  assert(n_continuations <= expected_continuations);

  if (n_continuations < expected_continuations)
    /* this continuation is incomplete: truncate here */
    *leading = 0;

  /* now the string must be completely valid */
  assert(ValidateUTF8(p));
}

std::pair<unsigned, const char *>
NextUTF8(const char *p)
{
  assert(p != nullptr);

  unsigned char a = *p++;
  if (a == 0)
    return std::make_pair(0u, nullptr);

  if (IsASCII(a))
    return std::make_pair(unsigned(a), p);

  assert(!IsContinuation(a));

  if (IsLeading1(a)) {
    /* 1 continuation */
    unsigned char b = *p++;
    assert(IsContinuation(b));

    return std::make_pair(((a & 0x1f) << 6) | (b & 0x3f), p);
  } else if (IsLeading2(a)) {
    /* 2 continuations */
    unsigned char b = *p++;
    assert(IsContinuation(b));
    unsigned char c = *p++;
    assert(IsContinuation(c));

    return std::make_pair(((a & 0xf) << 12) | ((b & 0x3f) << 6) | (c & 0x3f),
                          p);
  } else if (IsLeading3(a)) {
    /* 3 continuations */
    unsigned char b = *p++;
    assert(IsContinuation(b));
    unsigned char c = *p++;
    assert(IsContinuation(c));
    unsigned char d = *p++;
    assert(IsContinuation(d));

    return std::make_pair(((a & 0x7) << 18) | ((b & 0x3f) << 12)
                          | ((c & 0x3f) << 6) | (d & 0x3f),
                          p);
  } else if (IsLeading4(a)) {
    /* 4 continuations */
    unsigned char b = *p++;
    assert(IsContinuation(b));
    unsigned char c = *p++;
    assert(IsContinuation(c));
    unsigned char d = *p++;
    assert(IsContinuation(d));
    unsigned char e = *p++;
    assert(IsContinuation(e));

    return std::make_pair(((a & 0x3) << 24) | ((b & 0x3f) << 18)
                          | ((c & 0x3f) << 12) | ((d & 0x3f) << 6)
                          | (e & 0x3f),
                          p);
  } else if (IsLeading5(a)) {
    /* 5 continuations */
    unsigned char b = *p++;
    unsigned char c = *p++;
    unsigned char d = *p++;
    unsigned char e = *p++;
    unsigned char f = *p++;
    assert(IsContinuation(b));
    assert(IsContinuation(c));
    assert(IsContinuation(d));
    assert(IsContinuation(e));
    assert(IsContinuation(f));

    return std::make_pair(((a & 0x1) << 30) | ((b & 0x3f) << 24)
                          | ((c & 0x3f) << 18) | ((d & 0x3f) << 12)
                          | ((e & 0x3f) << 6) | (f & 0x3f),
                          p);
  } else {
    assert(false);
    gcc_unreachable();
  }
}
