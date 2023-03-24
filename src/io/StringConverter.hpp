// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Charset.hpp"
#include "util/ReusableArray.hpp"

#include <tchar.h>

/**
 * Helper which imports strings from a file to `TCHAR*`.
 */
class StringConverter {
  Charset charset;

  ReusableArray<TCHAR> tbuffer;

public:
  explicit StringConverter(Charset cs=Charset::AUTO) noexcept
    :charset(cs) {}

  /**
   * Does the given string start with the UTF-8 byte order mark?  This
   * is often a prefix which marks a file/string as UTF-8.
   */
  [[gnu::pure]]
  static bool IsByteOrderMark(const char *s) noexcept {
    return s[0] == (char)0xef && s[1] == (char)0xbb && s[2] == (char)0xbf;
  }

  [[gnu::pure]]
  static char *SkipByteOrderMark(char *s) noexcept {
    return IsByteOrderMark(s) ? s + 3 : nullptr;
  }

  bool IsAuto() const noexcept {
    return charset == Charset::AUTO;
  }

  void SetCharset(Charset _charset) noexcept {
    charset = _charset;
  }

  /**
   * Convert the given string.  The returned pointer may be the given
   * pointer or owned by this class and will be invalidated by the
   * next Convert() call.
   *
   * Throws on error.
   */
  TCHAR *Convert(char *src);
};
