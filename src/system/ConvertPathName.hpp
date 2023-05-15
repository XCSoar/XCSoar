// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Path.hpp"
#include "util/Compiler.h"

#ifdef _UNICODE
#include "util/ConvertString.hpp"
#include "util/LightString.hxx"

#include <wchar.h>
#else
#include "util/StringPointer.hxx"
#endif

/**
 * Representation of a file name.  It is automatically converted to
 * the file system character set.  If no conversion is needed, then
 * this object will hold a pointer to the original input string; it
 * must not be Invalidated.
 */
class PathName {
#ifdef _UNICODE
  typedef LightString<wchar_t> Value;
#else
  typedef StringPointer<> Value;
#endif

  Value value;

public:
  explicit PathName(Value::const_pointer _value) noexcept
    :value(_value) {}

#ifdef _UNICODE
  explicit PathName(const char *_value) noexcept
    :value(ConvertACPToWide(_value)) {}
#endif

public:
  bool IsDefined() const noexcept {
    return !value.IsNull();
  }

  operator Path() const noexcept {
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
  explicit NarrowPathName(Path _value) noexcept
    :value(ConvertWideToACP(_value.c_str())) {}
#else
  explicit NarrowPathName(Path _value) noexcept
    :value(_value.c_str()) {}
#endif

public:
  bool IsDefined() const noexcept {
    return !value.IsNull();
  }

  operator Value::const_pointer() const noexcept {
    return value.c_str();
  }
};
