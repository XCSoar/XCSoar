/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_IO_STRING_CONVERTER_HPP
#define XCSOAR_IO_STRING_CONVERTER_HPP

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

#endif
