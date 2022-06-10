/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#ifndef XCSOAR_CONVERT_STRING_HPP
#define XCSOAR_CONVERT_STRING_HPP

/** \file */

#include "UTF8.hpp"
#include "Compiler.h"

#ifdef _UNICODE
#include "AllocatedString.hxx"
#else
#include "StringPointer.hxx"
#endif

#include <cassert>

#ifdef _UNICODE

#include <cwchar>

/**
 * @return nullptr on error
 */
gcc_nonnull_all
BasicAllocatedString<wchar_t>
ConvertUTF8ToWide(const char *p) noexcept;

/**
 * @return nullptr on error
 */
gcc_nonnull_all
BasicAllocatedString<wchar_t>
ConvertACPToWide(const char *p) noexcept;

/**
 * @return nullptr on error
 */
gcc_nonnull_all
AllocatedString
ConvertWideToUTF8(const wchar_t *p) noexcept;

/**
 * @return nullptr on error
 */
gcc_nonnull_all
AllocatedString
ConvertWideToACP(const wchar_t *p) noexcept;

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
#else
  UTF8ToWideConverter(const_pointer _value) noexcept
    :value(_value)
  {
    assert(_value != nullptr);
  }
#endif

  UTF8ToWideConverter(const UTF8ToWideConverter &other) = delete;
  UTF8ToWideConverter &operator=(const UTF8ToWideConverter &other) = delete;

  gcc_pure
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
#else
  WideToUTF8Converter(const_pointer _value) noexcept
    :value(_value)
  {
    assert(_value != nullptr);
  }
#endif

  WideToUTF8Converter(const WideToUTF8Converter &other) = delete;
  WideToUTF8Converter &operator=(const WideToUTF8Converter &other) = delete;

  gcc_pure
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

  gcc_pure
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

#endif
