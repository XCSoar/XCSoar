// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Charset.hpp"
#include "util/ReusableArray.hpp"
#include "util/tstring_view.hxx"

#include <tchar.h>

/**
 * Helper which imports strings from a file to `char*`.
 */
class StringConverter {
  Charset charset;

  ReusableArray<char> tbuffer;

public:
  explicit StringConverter(Charset cs=Charset::AUTO) noexcept
    :charset(cs) {}

  bool IsAuto() const noexcept {
    return charset == Charset::AUTO;
  }

  void SetCharset(Charset _charset) noexcept {
    charset = _charset;
  }

  /**
   * Feed a string from the file and attempt to auto-detect the
   * charset.  Returns a pointer into the string after a byte-order
   * marker.
   */
  [[gnu::pure]]
  char *DetectStrip(char *src) noexcept;

  [[gnu::pure]]
  std::string_view DetectStrip(std::string_view src) noexcept;

  /**
   * Convert the given string.  The returned pointer may be the given
   * pointer or owned by this class and will be invalidated by the
   * next Convert() call.
   *
   * Throws on error.
   */
  char *Convert(char *src);

  tstring_view Convert(std::string_view src);
};
