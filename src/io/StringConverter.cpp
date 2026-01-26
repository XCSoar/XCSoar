// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StringConverter.hpp"
#include "util/Compiler.h"
#include "util/StringCompare.hxx"
#include "util/UTF8.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>


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

std::string_view
StringConverter::DetectStrip(std::string_view src) noexcept
{
  // Check if there is byte order mark in front
  if (charset == Charset::AUTO || charset == Charset::UTF8) {
    if (src.starts_with(utf8_byte_order_mark)) {
      src.remove_prefix(utf8_byte_order_mark.size());

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

  switch (charset) {
    size_t buffer_size;
    const char *utf8;

  case Charset::ISO_LATIN_1:
    buffer_size = strlen(narrow) * 2 + 1;
    utf8 = Latin1ToUTF8(narrow, {tbuffer.get(buffer_size), buffer_size});
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
}

tstring_view
StringConverter::Convert(std::string_view src)
{
  src = DetectStrip(src);
  if (src.empty())
    return {};

  switch (charset) {
    size_t buffer_size;

  case Charset::ISO_LATIN_1:
    buffer_size = src.size() * 2;

    if (const auto utf8 = Latin1ToUTF8(src, {tbuffer.get(buffer_size), buffer_size});
        utf8.data() != nullptr)
      return utf8;
    else
      throw std::runtime_error("Latin-1 to UTF-8 conversion failed");

  case Charset::UTF8:
    if (!ValidateUTF8(src))
      /* abort on invalid UTF-8 sequence */
      throw std::runtime_error("Invalid UTF-8");

    [[fallthrough]];

  case Charset::AUTO:
    return src;
  }

  /* unreachable */
  gcc_unreachable();
}
