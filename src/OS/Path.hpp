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

#ifndef OS_ALLOCATED_PATH_HPP
#define OS_ALLOCATED_PATH_HPP

#include "Util/StringPointer.hxx"
#include "Util/AllocatedString.hxx"
#include "Compiler.h"

#include <string>

#ifdef _UNICODE
#include <tchar.h>
#endif

#include <cstddef>

class AllocatedPath;

/**
 * The path name of a local file.
 *
 * This class points to memory managed by somebody else.  It is used
 * as replacement for regular C strings, for passing paths around.
 * For a managed variant, see #AllocatedPath.
 */
class Path {
public:
#ifdef _UNICODE
  typedef TCHAR char_type;
#else
  typedef char char_type;
#endif
  typedef StringPointer<char_type> value_type;
  typedef value_type::const_pointer const_pointer;
  typedef value_type::pointer pointer;

  static constexpr auto SENTINEL = value_type::SENTINEL;

private:
  value_type value;

public:
  Path() = default;
  explicit constexpr Path(const_pointer _value):value(_value) {}
  Path(std::nullptr_t n):value(n) {}

  gcc_pure
  AllocatedPath operator+(const_pointer other) const;

  constexpr bool IsNull() const {
    return value.IsNull();
  }

  bool IsEmpty() const {
    return value.empty();
  }

  constexpr const_pointer c_str() const {
    return value.c_str();
  }

  /**
   * Convert the path to UTF-8.
   * Returns empty string on error or if this instance is "nulled"
   * (#IsNull returns true).
   */
  gcc_pure
  std::string ToUTF8() const;

  gcc_pure
  bool operator==(Path other) const;

  gcc_pure
  bool operator!=(Path other) const {
    return !(*this == other);
  }

  constexpr bool operator==(std::nullptr_t) const {
    return value.IsNull();
  }

  constexpr bool operator!=(std::nullptr_t) const {
    return !value.IsNull();
  }

  gcc_pure
  bool IsAbsolute() const;

  /**
   * Is this path a "base name", i.e. is there no path separate?
   * Behaviour is undefined when the string is empty.
   */
  gcc_pure
  bool IsBase() const;

  /**
   * Returns the parent of the specified path, i.e. the part before
   * the last separator.  Returns "." if there is no directory name.
   */
  gcc_pure
  AllocatedPath GetParent() const;

  /**
   * Returns the base name of the specified path, i.e. the part after
   * the last separator.  May return nullptr if there is no base name.
   */
  gcc_pure
  Path GetBase() const;

  /**
   * Check if this object is "inside" to the given path, and if yes,
   * return the relative path.
   */
  gcc_pure
  Path RelativeTo(Path parent) const;

  gcc_pure
  bool MatchesExtension(const_pointer extension) const;

  /**
   * Returns the filename extension (starting with a dot) or nullptr
   * if the base name doesn't have one.
   */
  gcc_pure
  const_pointer GetExtension() const;

  /**
   * Return the path with its filename extension replaced with the given one.
   *
   * @param new_extension the new filename extension (must start with
   * a dot)
   */
  gcc_pure
  AllocatedPath WithExtension(const_pointer new_extension) const;
};

/**
 * The path name of a local file.
 *
 * This class points manages its string memory.  It is heavier than
 * class #Path.
 */
class AllocatedPath {
  friend class Path;

public:
  typedef Path::char_type char_type;
  typedef Path::const_pointer const_pointer;
  typedef Path::pointer pointer;
  typedef AllocatedString<char_type> value_type;

  static constexpr auto SENTINEL = value_type::SENTINEL;

private:
  value_type value;

  AllocatedPath(size_t size):value(value_type::Donate(new char_type[size])) {}

  AllocatedPath(value_type &&src):value(std::move(src)) {}

public:
  AllocatedPath(AllocatedPath &&) = default;

  AllocatedPath(std::nullptr_t n):value(n) {}

  AllocatedPath(Path src)
    :value(src.IsNull() ? nullptr : value_type::Duplicate(src.c_str())) {}

  explicit AllocatedPath(const_pointer src)
    :AllocatedPath(Path(src)) {}

  AllocatedPath(const_pointer _begin, const_pointer _end)
    :AllocatedPath(value_type::Duplicate(_begin, _end)) {}

  static AllocatedPath Donate(pointer value) {
    return value_type::Donate(value);
  }

  gcc_pure
  static AllocatedPath Build(const_pointer a, const_pointer b);

  gcc_pure
  static AllocatedPath Build(Path a, const_pointer b) {
    return Build(a.c_str(), b);
  }

  gcc_pure
  static AllocatedPath Build(Path a, Path b) {
    return Build(a, b.c_str());
  }

  AllocatedPath &operator=(AllocatedPath &&) = default;

  AllocatedPath &operator=(std::nullptr_t n) {
    value = n;
    return *this;
  }

  AllocatedPath &operator=(Path src) {
    return *this = AllocatedPath(src);
  }

  gcc_pure
  AllocatedPath operator+(const_pointer other) const {
    return Path(*this) + other;
  }

  bool IsNull() const {
    return value.IsNull();
  }

  bool IsEmpty() const {
    return value.empty();
  }

  const_pointer c_str() const {
    return value.c_str();
  }

  std::string ToUTF8() const {
    return Path(*this).ToUTF8();
  }

  gcc_pure
  bool operator==(Path other) const {
    return Path(*this) == other;
  }

  gcc_pure
  bool operator!=(Path other) const {
    return !(*this == other);
  }

  gcc_pure
  bool operator==(std::nullptr_t) const {
    return value.IsNull();
  }

  gcc_pure
  bool operator!=(std::nullptr_t) const {
    return !value.IsNull();
  }

  operator Path() const {
    return Path(c_str());
  }

  gcc_pure
  bool IsAbsolute() const {
    return Path(*this).IsAbsolute();
  }

  gcc_pure
  bool IsBase() const {
    return Path(*this).IsBase();
  }

  AllocatedPath GetParent() const {
    return Path(*this).GetParent();
  }

  gcc_pure
  Path GetBase() const {
    return Path(*this).GetBase();
  }

  /**
   * Check if this object is "inside" to the given path, and if yes,
   * return the relative path.
   */
  gcc_pure
  Path RelativeTo(Path parent) const {
    return Path(*this).RelativeTo(parent);
  }

  gcc_pure
  bool MatchesExtension(const_pointer extension) const {
    return Path(*this).MatchesExtension(extension);
  }

  gcc_pure
  const_pointer GetExtension() const {
    return Path(*this).GetExtension();
  }

  gcc_pure
  AllocatedPath WithExtension(const_pointer new_extension) const {
    return Path(*this).WithExtension(new_extension);
  }
};

#endif
