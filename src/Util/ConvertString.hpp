/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "UTF8.hpp"
#include "Compiler.h"

#include <assert.h>
#include <string.h>
#include <tchar.h>

#ifdef _UNICODE

gcc_malloc gcc_nonnull_all
TCHAR *
ConvertUTF8ToWide(const char *p);

gcc_malloc gcc_nonnull_all
TCHAR *
ConvertACPToWide(const char *p);

gcc_malloc gcc_nonnull_all
char *
ConvertWideToUTF8(const TCHAR *p);

gcc_malloc gcc_nonnull_all
char *
ConvertWideToACP(const TCHAR *p);

#endif

/**
 * Convert a UTF-8 string to a TCHAR string.  The source buffer passed
 * to the constructor must be valid as long as this object is being
 * used.
 */
class UTF8ToWideConverter {
#ifdef _UNICODE
  TCHAR *value;
#else
  const char *value;
#endif

public:
#ifdef _UNICODE
  UTF8ToWideConverter(const char *_value)
    :value(ConvertUTF8ToWide(_value)) {}

  ~UTF8ToWideConverter() {
    delete[] value;
  }
#else
  UTF8ToWideConverter(const char *_value):value(_value) {
    assert(_value != nullptr);
  }
#endif

  UTF8ToWideConverter(const UTF8ToWideConverter &other) = delete;
  UTF8ToWideConverter &operator=(const UTF8ToWideConverter &other) = delete;

  gcc_pure
  bool IsValid() const {
#ifdef _UNICODE
    return value != nullptr;
#else
    assert(value != nullptr);

    return ValidateUTF8(value);
#endif
  }

  operator const TCHAR *() const {
    assert(value != nullptr);

    return value;
  }
};

/**
 * Convert a TCHAR string to UTF-8.  The source buffer passed to the
 * constructor must be valid as long as this object is being used.
 */
class WideToUTF8Converter {
#ifdef _UNICODE
  char *value;
#else
  const char *value;
#endif

public:
#ifdef _UNICODE
  WideToUTF8Converter(const TCHAR *_value)
    :value(ConvertWideToUTF8(_value)) {}

  ~WideToUTF8Converter() {
    delete[] value;
  }
#else
  WideToUTF8Converter(const char *_value):value(_value) {
    assert(_value != nullptr);
  }
#endif

  WideToUTF8Converter(const WideToUTF8Converter &other) = delete;
  WideToUTF8Converter &operator=(const WideToUTF8Converter &other) = delete;

  gcc_pure
  bool IsValid() const {
#ifdef _UNICODE
    return value != nullptr;
#else
    assert(value != nullptr);

    return true;
#endif
  }

  operator const char *() const {
    assert(value != nullptr);

    return value;
  }
};

/**
 * Convert an ACP string (Windows ANSI code page) to a TCHAR string.
 * The source buffer passed to the constructor must be valid as long
 * as this object is being used.
 */
class ACPToWideConverter {
#ifdef _UNICODE
  TCHAR *value;
#else
  const char *value;
#endif

public:
#ifdef _UNICODE
  ACPToWideConverter(const char *_value)
    :value(ConvertACPToWide(_value)) {}

  ~ACPToWideConverter() {
    delete[] value;
  }
#else
  ACPToWideConverter(const char *_value):value(_value) {
    assert(_value != nullptr);
  }
#endif

  ACPToWideConverter(const ACPToWideConverter &other) = delete;
  ACPToWideConverter &operator=(const ACPToWideConverter &other) = delete;

  gcc_pure
  bool IsValid() const {
#ifdef _UNICODE
    return value != nullptr;
#else
    assert(value != nullptr);

    return true;
#endif
  }

  operator const TCHAR *() const {
    assert(value != nullptr);

    return value;
  }

  /**
   * Returns a newly allocated string.  It invalidates this object.
   */
  gcc_malloc
  TCHAR *StealDup() {
    assert(value != nullptr);

#ifdef _UNICODE
    TCHAR *result = value;
    value = nullptr;
    return result;
#else
    return strdup(value);
#endif
  }
};

/**
 * Convert a TCHAR string to ACP (Windows ANSI code page).  The source
 * buffer passed to the constructor must be valid as long as this
 * object is being used.
 */
class WideToACPConverter {
#ifdef _UNICODE
  char *value;
#else
  const char *value;
#endif

public:
#ifdef _UNICODE
  WideToACPConverter(const TCHAR *_value)
    :value(ConvertWideToACP(_value)) {}

  ~WideToACPConverter() {
    delete[] value;
  }
#else
  WideToACPConverter(const char *_value):value(_value) {
    assert(_value != nullptr);
  }
#endif

  WideToACPConverter(const WideToACPConverter &other) = delete;
  WideToACPConverter &operator=(const WideToACPConverter &other) = delete;

  gcc_pure
  bool IsValid() const {
#ifdef _UNICODE
    return value != nullptr;
#else
    assert(value != nullptr);

    return true;
#endif
  }

  operator const char *() const {
    assert(value != nullptr);

    return value;
  }

  /**
   * Returns a newly allocated string.  It invalidates this object.
   */
  gcc_malloc
  char *StealDup() {
    assert(value != nullptr);

#ifdef _UNICODE
    char *result = value;
    value = nullptr;
    return result;
#else
    return strdup(value);
#endif
  }
};

#endif
