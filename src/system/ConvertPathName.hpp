// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Path.hpp"
#include "util/Compiler.h"
#include "util/StringPointer.hxx"

/**
 * Representation of a file name.  It is automatically converted to
 * the file system character set.  If no conversion is needed, then
 * this object will hold a pointer to the original input string; it
 * must not be Invalidated.
 */
class PathName {
  typedef StringPointer<> Value;

  Value value;

public:
  explicit PathName(Value::const_pointer _value) noexcept
    :value(_value) {}

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
  typedef StringPointer<> Value;

  Value value;

public:
  explicit NarrowPathName(Path _value) noexcept
    :value(_value.c_str()) {}

public:
  bool IsDefined() const noexcept {
    return !value.IsNull();
  }

  operator Value::const_pointer() const noexcept {
    return value.c_str();
  }
};
