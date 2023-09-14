// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StringConverter.hpp"
#include "util/Compiler.h"
#include "util/StringCompare.hxx"
#include "util/UTF8.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>

#ifdef _UNICODE
#include "system/Error.hxx"
#include <stringapiset.h>
#endif

#ifdef _UNICODE

static constexpr void
iso_latin_1_to_tchar(TCHAR *dest, std::string_view src) noexcept
{
  for (unsigned char ch : src)
    *dest++ = ch;
  *dest = _T('\0');
}

#endif

char *
StringConverter::DetectStrip(char *src) noexcept
{
  assert(src != nullptr);

  // Check if there is byte order mark in front
  if (charset == Charset::AUTO || charset == Charset::UTF8) {
    if (StringStartsWith(src, utf8_byte_order_mark)) {
      src += utf8_byte_order_mark.size();

      /* switch to UTF-8 now */
      charset = Charset::UTF8;
    }
  }

  if (charset == Charset::AUTO && !ValidateUTF8(src))
    /* invalid UTF-8 sequence detected: switch to ISO-Latin-1 */
    charset = Charset::ISO_LATIN_1;

  return src;
}

TCHAR *
StringConverter::Convert(char *narrow)
{
  narrow = DetectStrip(narrow);

#ifdef _UNICODE
  const std::string_view src{narrow};

  TCHAR *t = tbuffer.get(src.size() + 1);
  assert(t != nullptr);

  if (src.empty()) {
    t[0] = _T('\0');
    return t;
  }

  switch (charset) {
  case Charset::ISO_LATIN_1:
    iso_latin_1_to_tchar(t, src);
    break;

  default:
    int length = MultiByteToWideChar(CP_UTF8, 0, src.data(), src.size(),
                                     t, src.size());
    if (length == 0)
      throw MakeLastError("Failed to convert string");

    t[length] = _T('\0');

    break;
  }

  return t;
#else
  switch (charset) {
    size_t buffer_size;
    const char *utf8;

  case Charset::ISO_LATIN_1:
    buffer_size = strlen(narrow) * 2 + 1;
    utf8 = Latin1ToUTF8(narrow, tbuffer.get(buffer_size), buffer_size);
    if (utf8 == nullptr)
      throw std::runtime_error("Latin-1 to UTF-8 conversion failed");

    return const_cast<char *>(utf8);

  case Charset::UTF8:
    if (!ValidateUTF8(narrow))
      /* abort on invalid UTF-8 sequence */
      throw std::runtime_error("Invalid UTF-8");

    [[fallthrough]];

  case Charset::AUTO:
    return narrow;
  }

  /* unreachable */
  gcc_unreachable();
#endif
}
