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

#ifndef OS_CONVERT_PATH_NAME_HPP
#define OS_CONVERT_PATH_NAME_HPP

#include "Compiler.h"

#ifdef _UNICODE
#include "Util/ConvertString.hpp"
#endif

#include <tchar.h>

/**
 * Representation of a file name.  It is automatically converted to
 * the file system character set.  If no conversion is needed, then
 * this object will hold a pointer to the original input string; it
 * must not be Invalidated.
 */
class PathName {
#ifdef _UNICODE
  TCHAR *allocated;
#endif
  const TCHAR *value;

public:
#ifdef _UNICODE
  explicit PathName(const TCHAR *_value)
    :allocated(NULL), value(_value) {}

  explicit PathName(const char *_value)
    :allocated(ConvertACPToWide(_value)), value(allocated) {}

  ~PathName() {
    delete[] allocated;
  }
#else /* !_UNICODE */
  explicit PathName(const TCHAR *_value):value(_value) {}
#endif /* !_UNICODE */

public:
  bool IsDefined() const {
#ifdef _UNICODE
    return value != NULL;
#else
    return true;
#endif
  }

  operator const TCHAR *() const {
    return value;
  }
};

/**
 * Representation of a file name in narrow characters.  If no
 * conversion is needed, then this object will hold a pointer to the
 * original input string; it must not be Invalidated.
 */
class NarrowPathName {
#ifdef _UNICODE
  char *allocated;
#endif
  const char *value;

public:
#ifdef _UNICODE
  explicit NarrowPathName(const char *_value)
    :allocated(NULL), value(_value) {}

  explicit NarrowPathName(const TCHAR *_value)
    :allocated(ConvertWideToACP(_value)), value(allocated) {}

  ~NarrowPathName() {
    delete[] allocated;
  }
#else /* !_UNICODE */
  explicit NarrowPathName(const char *_value):value(_value) {}
#endif /* !_UNICODE */

public:
  bool IsDefined() const {
#ifdef _UNICODE
    return value != NULL;
#else
    return true;
#endif
  }

  operator const char *() const {
    return value;
  }
};

#endif
