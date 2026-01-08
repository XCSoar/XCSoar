// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "UTF8.hpp"

#ifdef _UNICODE
#include "AllocatedString.hxx"
#include <concepts>
#else
#include "StringPointer.hxx"
#endif

#include <cassert>

#ifdef _UNICODE

#include <cwchar>

/**
 * @return nullptr on error
 */
[[gnu::nonnull]]
BasicAllocatedString<wchar_t>
ConvertUTF8ToWide(const char *p) noexcept;

/**
 * @return nullptr on error
 */
[[gnu::nonnull]]
BasicAllocatedString<wchar_t>
ConvertACPToWide(const char *p) noexcept;

/**
 * @return nullptr on error
 */
[[gnu::nonnull]]
AllocatedString
ConvertWideToUTF8(const wchar_t *p) noexcept;

/**
 * @return nullptr on error
 */
AllocatedString
ConvertWideToUTF8(std::wstring_view p) noexcept;

/**
 * @return nullptr on error
 */
[[gnu::nonnull]]
AllocatedString
ConvertWideToACP(const wchar_t *p) noexcept;

/**
 * @return nullptr on error
 */
[[gnu::nonnull]]
BasicAllocatedString<wchar_t>
ConvertUTF8ToWide(std::string_view p) noexcept;

#endif

/**
 * Convert a UTF-8 string to a TCHAR string.  The source buffer passed
 * to the constructor must be valid as long as this object is being
 * used.
 */
class UTF8ToWideConverter {
#ifdef _UNICODE
  typedef BasicAllocatedString<wchar_t> Value;
#else
  typedef StringPointer<> Value;
#endif
  typedef typename Value::const_pointer const_pointer;

  Value value;

public:
#ifdef _UNICODE
  UTF8ToWideConverter(const char *_value) noexcept
    :value(ConvertUTF8ToWide(_value)) {}

  /**
   * Constructor for std::string_view. Uses a template with SFINAE to avoid
   * ambiguity with types that can convert to both char* and string_view.
   */
  template<typename T>
  requires std::same_as<T, std::string_view>
  UTF8ToWideConverter(T _value) noexcept
    :value(ConvertUTF8ToWide(_value)) {}
#else
  UTF8ToWideConverter(const_pointer _value) noexcept
    :value(_value)
  {
    assert(_value != nullptr);
  }
#endif

  UTF8ToWideConverter(const UTF8ToWideConverter &other) = delete;
  UTF8ToWideConverter &operator=(const UTF8ToWideConverter &other) = delete;

  [[gnu::pure]]
  bool IsValid() const noexcept {
#ifdef _UNICODE
    return value != nullptr;
#else
    assert(value != nullptr);

    return ValidateUTF8(value.c_str());
#endif
  }

  const_pointer c_str() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }

  operator const_pointer() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }

#ifdef _UNICODE
  /**
   * Explicit conversion to std::wstring_view. Use this when passing
   * to functions that take std::wstring_view or tstring_view.
   */
  std::wstring_view sv() const noexcept {
    return value;
  }
#endif
};

/**
 * Convert a TCHAR string to UTF-8.  The source buffer passed to the
 * constructor must be valid as long as this object is being used.
 */
class WideToUTF8Converter {
#ifdef _UNICODE
  typedef AllocatedString Value;
#else
  typedef StringPointer<> Value;
#endif
  typedef typename Value::const_pointer const_pointer;

  Value value;

public:
#ifdef _UNICODE
  WideToUTF8Converter(const wchar_t *_value) noexcept
    :value(ConvertWideToUTF8(_value)) {}

  /**
   * Constructor for std::wstring_view. Uses a template with SFINAE to avoid
   * ambiguity with types that can convert to both wchar_t* and wstring_view
   * (like StaticString).
   */
  template<typename T>
  requires std::same_as<T, std::wstring_view>
  WideToUTF8Converter(T _value) noexcept
    :value(ConvertWideToUTF8(_value)) {}
#else
  WideToUTF8Converter(const_pointer _value) noexcept
    :value(_value)
  {
    assert(_value != nullptr);
  }
#endif

  WideToUTF8Converter(const WideToUTF8Converter &other) = delete;
  WideToUTF8Converter &operator=(const WideToUTF8Converter &other) = delete;

  [[gnu::pure]]
  bool IsValid() const noexcept {
#ifdef _UNICODE
    return value != nullptr;
#else
    assert(value != nullptr);

    return true;
#endif
  }

  const_pointer c_str() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }

  operator const_pointer() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }

#ifdef _UNICODE
  /**
   * Explicit conversion to std::string_view. Use this when passing
   * to functions that take std::string_view.
   */
  std::string_view sv() const noexcept {
    return value;
  }
#endif
};

/**
 * Convert a TCHAR string to ACP (Windows ANSI code page).  The source
 * buffer passed to the constructor must be valid as long as this
 * object is being used.
 */
class WideToACPConverter {
#ifdef _UNICODE
  typedef AllocatedString Value;
#else
  typedef StringPointer<> Value;
#endif
  typedef typename Value::const_pointer const_pointer;

  Value value;

public:
#ifdef _UNICODE
  WideToACPConverter(const wchar_t *_value) noexcept
    :value(ConvertWideToACP(_value)) {}
#else
  WideToACPConverter(const_pointer _value) noexcept
    :value(_value)
  {
    assert(_value != nullptr);
  }
#endif

  WideToACPConverter(const WideToACPConverter &other) = delete;
  WideToACPConverter &operator=(const WideToACPConverter &other) = delete;

  [[gnu::pure]]
  bool IsValid() const noexcept {
#ifdef _UNICODE
    return value != nullptr;
#else
    assert(value != nullptr);

    return true;
#endif
  }

  const_pointer c_str() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }

  operator const_pointer() const noexcept {
    assert(value != nullptr);

    return value.c_str();
  }
};
