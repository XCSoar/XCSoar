/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef OS_CONVERT_PATH_NAME_HPP
#define OS_CONVERT_PATH_NAME_HPP

#include "Path.hpp"
#include "Compiler.h"

#ifdef _UNICODE
#include "Util/ConvertString.hpp"
#include "Util/LightString.hxx"

#include <tchar.h>
#else
#include "Util/StringPointer.hxx"
#endif

/**
 * Representation of a file name.  It is automatically converted to
 * the file system character set.  If no conversion is needed, then
 * this object will hold a pointer to the original input string; it
 * must not be Invalidated.
 */
class PathName {
#ifdef _UNICODE
  typedef LightString<TCHAR> Value;
#else
  typedef StringPointer<> Value;
#endif

  Value value;

public:
  explicit PathName(Value::const_pointer _value)
    :value(_value) {}

#ifdef _UNICODE
  explicit PathName(const char *_value)
    :value(Value::Donate(ConvertACPToWide(_value))) {}
#endif

public:
  bool IsDefined() const {
    return !value.IsNull();
  }

  operator Path() const {
    return Path(value.c_str());
  }
};

/**
 * Representation of a file name in narrow characters.  If no
 * conversion is needed, then this object will hold a pointer to the
 * original input string; it must not be Invalidated.
 */
class NarrowPathName {
#ifdef _UNICODE
  typedef LightString<char> Value;
#else
  typedef StringPointer<> Value;
#endif

  Value value;

public:
#ifdef _UNICODE
  explicit NarrowPathName(Path _value)
    :value(Value::Donate(ConvertWideToACP(_value.c_str()))) {}
#else
  explicit NarrowPathName(Path _value)
    :value(_value.c_str()) {}
#endif

public:
  bool IsDefined() const {
    return !value.IsNull();
  }

  operator Value::const_pointer() const {
    return value.c_str();
  }
};

#endif
