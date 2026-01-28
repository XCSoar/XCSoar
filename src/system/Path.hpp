// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
  using char_type = wchar_t;
#else
  using char_type = char;
#endif
  using value_type = StringPointer<char_type>;
  using const_pointer = value_type::const_pointer;
  using pointer = value_type::pointer;

  static constexpr auto SENTINEL = value_type::SENTINEL;

private:
  value_type value;

public:
  Path() = default;
  explicit constexpr Path(const_pointer _value) noexcept:value(_value) {}
  Path(std::nullptr_t n):value(n) {}

  [[gnu::pure]]
  AllocatedPath operator+(const_pointer other) const noexcept;

  bool empty() const noexcept {
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
  bool EndsWithIgnoreCase(const_pointer needle) const noexcept;

  /**
   * Returns the filename suffix (starting with a dot) or nullptr if
   * the base name doesn't have one.
   */
  [[gnu::pure]]
  const_pointer GetSuffix() const noexcept;

  /**
   * Return the path with its filename suffix replaced with the given one.
   *
   * @param new_suffix the new filename suffix (must start with
   * a dot)
   */
  [[gnu::pure]]
  AllocatedPath WithSuffix(const_pointer new_suffix) const noexcept;
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
  using char_type = Path::char_type;
  using const_pointer = Path::const_pointer;
  using pointer = Path::pointer;
  using string_view = std::basic_string_view<char_type>;
  using value_type = BasicAllocatedString<char_type>;

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

  bool empty() const noexcept {
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
  bool EndsWithIgnoreCase(const_pointer needle) const noexcept {
    return Path{*this}.EndsWithIgnoreCase(needle);
  }

  [[gnu::pure]]
  const_pointer GetSuffix() const noexcept {
    return Path{*this}.GetSuffix();
  }

  [[gnu::pure]]
  AllocatedPath WithSuffix(const_pointer new_suffix) const noexcept {
    return Path{*this}.WithSuffix(new_suffix);
  }
};

/**
 * Append ".partial" extension to a path.
 *
 * Used for temporary files during downloads/transfers.
 */
[[gnu::pure]]
AllocatedPath MakePartialPath(Path path) noexcept;
