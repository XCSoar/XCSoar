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

#ifndef OS_ALLOCATED_PATH_HPP
#define OS_ALLOCATED_PATH_HPP

#include "util/StringPointer.hxx"
#include "util/AllocatedString.hxx"

#include <string>
#include <string_view>

#ifdef _UNICODE
#include <wchar.h>
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
  typedef wchar_t char_type;
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
  explicit constexpr Path(const_pointer _value) noexcept:value(_value) {}
  Path(std::nullptr_t n):value(n) {}

  [[gnu::pure]]
  AllocatedPath operator+(const_pointer other) const noexcept;

  bool IsEmpty() const noexcept {
    return value.empty();
  }

  constexpr const_pointer c_str() const noexcept {
    return value.c_str();
  }

  /**
   * Convert the path to UTF-8.
   * Returns empty string on error or if this instance is "nulled".
   */
  [[gnu::pure]]
  std::string ToUTF8() const noexcept;

  [[gnu::pure]]
  bool operator==(Path other) const noexcept;

  [[gnu::pure]]
  bool operator!=(Path other) const noexcept {
    return !(*this == other);
  }

  constexpr bool operator==(std::nullptr_t n) const noexcept {
    return value == n;
  }

  constexpr bool operator!=(std::nullptr_t n) const noexcept {
    return value != n;
  }

  [[gnu::pure]]
  bool IsAbsolute() const noexcept;

  /**
   * Is this path a "base name", i.e. is there no path separate?
   * Behaviour is undefined when the string is empty.
   */
  [[gnu::pure]]
  bool IsBase() const noexcept;

  /**
   * Returns the parent of the specified path, i.e. the part before
   * the last separator.  Returns "." if there is no directory name.
   */
  [[gnu::pure]]
  AllocatedPath GetParent() const noexcept;

  /**
   * Returns the base name of the specified path, i.e. the part after
   * the last separator.  May return nullptr if there is no base name.
   */
  [[gnu::pure]]
  Path GetBase() const noexcept;

  /**
   * Check if this object is "inside" to the given path, and if yes,
   * return the relative path.
   */
  [[gnu::pure]]
  Path RelativeTo(Path parent) const noexcept;

  [[gnu::pure]]
  bool MatchesExtension(const_pointer extension) const noexcept;

  /**
   * Returns the filename extension (starting with a dot) or nullptr
   * if the base name doesn't have one.
   */
  [[gnu::pure]]
  const_pointer GetExtension() const noexcept;

  /**
   * Return the path with its filename extension replaced with the given one.
   *
   * @param new_extension the new filename extension (must start with
   * a dot)
   */
  [[gnu::pure]]
  AllocatedPath WithExtension(const_pointer new_extension) const noexcept;
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
  using string_view = std::basic_string_view<char_type>;
  typedef BasicAllocatedString<char_type> value_type;

  static constexpr auto SENTINEL = value_type::SENTINEL;

private:
  value_type value;

  AllocatedPath(size_t size) noexcept
    :value(value_type::Donate(new char_type[size])) {}

  AllocatedPath(value_type &&src) noexcept
    :value(std::move(src)) {}

public:
  AllocatedPath(AllocatedPath &&) = default;

  AllocatedPath() noexcept = default;
  AllocatedPath(std::nullptr_t n) noexcept:value(n) {}

  AllocatedPath(Path src) noexcept
    :value(src == nullptr ? nullptr : value_type(src.c_str())) {}

  explicit AllocatedPath(const_pointer src) noexcept
    :AllocatedPath(Path(src)) {}

  AllocatedPath(const_pointer _begin, const_pointer _end) noexcept
    :AllocatedPath(value_type({_begin, size_t(_end - _begin)})) {}

  static AllocatedPath Donate(pointer value) noexcept {
    return value_type::Donate(value);
  }

  [[gnu::pure]]
  static AllocatedPath Build(string_view a, string_view b) noexcept;

  [[gnu::pure]]
  static AllocatedPath Build(Path a, const_pointer b) noexcept {
    return Build(a.c_str(), b);
  }

  [[gnu::pure]]
  static AllocatedPath Build(Path a, Path b) noexcept {
    return Build(a, b.c_str());
  }

  AllocatedPath &operator=(AllocatedPath &&) = default;

  AllocatedPath &operator=(std::nullptr_t n) noexcept {
    value = n;
    return *this;
  }

  AllocatedPath &operator=(Path src) noexcept {
    return *this = AllocatedPath(src);
  }

  [[gnu::pure]]
  AllocatedPath operator+(const_pointer other) const noexcept {
    return Path(*this) + other;
  }

  bool IsEmpty() const noexcept {
    return value.empty();
  }

  const_pointer c_str() const noexcept {
    return value.c_str();
  }

  std::string ToUTF8() const noexcept {
    return Path(*this).ToUTF8();
  }

  [[gnu::pure]]
  bool operator==(Path other) const noexcept {
    return Path(*this) == other;
  }

  [[gnu::pure]]
  bool operator!=(Path other) const noexcept {
    return !(*this == other);
  }

  [[gnu::pure]]
  bool operator==(const AllocatedPath &other) const noexcept {
    return Path{*this} == Path{other};
  }

  [[gnu::pure]]
  bool operator!=(const AllocatedPath &other) const noexcept {
    return !(*this == other);
  }

  constexpr bool operator==(std::nullptr_t n) const noexcept {
    return value == n;
  }

  constexpr bool operator!=(std::nullptr_t n) const noexcept {
    return value != n;
  }

  operator Path() const noexcept {
    return Path(c_str());
  }

  [[gnu::pure]]
  bool IsAbsolute() const noexcept {
    return Path(*this).IsAbsolute();
  }

  [[gnu::pure]]
  bool IsBase() const noexcept {
    return Path(*this).IsBase();
  }

  AllocatedPath GetParent() const noexcept {
    return Path(*this).GetParent();
  }

  [[gnu::pure]]
  Path GetBase() const noexcept {
    return Path(*this).GetBase();
  }

  /**
   * Check if this object is "inside" to the given path, and if yes,
   * return the relative path.
   */
  [[gnu::pure]]
  Path RelativeTo(Path parent) const noexcept {
    return Path(*this).RelativeTo(parent);
  }

  [[gnu::pure]]
  bool MatchesExtension(const_pointer extension) const noexcept {
    return Path(*this).MatchesExtension(extension);
  }

  [[gnu::pure]]
  const_pointer GetExtension() const noexcept {
    return Path(*this).GetExtension();
  }

  [[gnu::pure]]
  AllocatedPath WithExtension(const_pointer new_extension) const noexcept {
    return Path(*this).WithExtension(new_extension);
  }
};

#endif
