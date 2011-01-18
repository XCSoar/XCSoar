/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_PROFILE_GCONF_HPP
#define XCSOAR_PROFILE_GCONF_HPP

#include "Math/fixed.hpp"

#include <tchar.h>
#include <cstdio>

class ProfileWriter;

namespace ProfileGConf {
  bool _Get(const TCHAR *szRegValue, int &pPos);

  bool Get(const TCHAR *szRegValue, TCHAR *pPos, size_t dwSize);
  bool Set(const TCHAR *szRegValue, const TCHAR *Pos);

  static inline bool Get(const TCHAR *key, int &value)
  {
    // Try to read the GConf key as a string
    TCHAR str[50];
    if (Get(key, str, 50)) {
      // Parse the string for a number
      TCHAR *endptr;
      int tmp = _tcstol(str, &endptr, 0);
      if (endptr != str) {
        // Save parsed value to output parameter value and return success
        value = tmp;
        return true;
      }
    }

    // Try to read the GConf key as integer
    int temp;
    if (!_Get(key, temp))
      return false;

    // Save the read DWORD as output parameter and return success
    value = temp;
    return true;
  }

  static inline bool Get(const TCHAR *key, short &value)
  {
    // Try to read the GConf key as a string
    TCHAR str[50];
    if (Get(key, str, 50)) {
      // Parse the string for a number
      TCHAR *endptr;
      short tmp = _tcstol(str, &endptr, 0);
      if (endptr != str) {
        // Save parsed value to output parameter value and return success
        value = tmp;
        return true;
      }
    }

    // Try to read the GConf key as integer
    int temp;
    if (!_Get(key, temp))
      return false;

    // Save the read DWORD as output parameter and return success
    value = (short)temp;
    return true;
  }

  static inline bool Get(const TCHAR *key, bool &value)
  {
    // Try to read the GConf key as a string
    TCHAR str[5];
    if (Get(key, str, 5)) {
      value = (str[0] != '0');
      return true;
    }

    // Try to read the GConf key as integer
    int temp;
    if (!_Get(key, temp))
      return false;

    // Save the read DWORD as output parameter and return success
    value = temp > 0;
    return true;
  }

  static inline bool Get(const TCHAR *key, unsigned &value)
  {
    // Try to read the GConf key as a string
    TCHAR str[50];
    if (Get(key, str, 50)) {
      // Parse the string for an unsigned number
      TCHAR *endptr;
      unsigned tmp = _tcstoul(str, &endptr, 0);
      if (endptr != str) {
        // Save parsed value to output parameter value and return success
        value = tmp;
        return true;
      }
    }

    // Try to read the GConf key as integer
    int temp;
    if (!_Get(key, temp))
      return false;

    // Save the read DWORD as output parameter and return success
    value = temp;
    return true;
  }

  static inline bool Get(const TCHAR *key, fixed &value)
  {
    // Try to read the GConf key as a string
    TCHAR str[50];
    if (Get(key, str, 50)) {
      // Parse the string for a floating point number
      TCHAR *endptr;
      double tmp = _tcstod(str, &endptr);
      if (endptr != str) {
        // Save parsed value to output parameter value and return success
        value = fixed(tmp);
        return true;
      }
    }

    // Try to read the GConf key as integer
    int temp;
    if (!_Get(key, temp))
      return false;

    // Save the read DWORD as output parameter and return success
    value = fixed(temp);
    return true;
  }

  static inline bool Set(const TCHAR *key, unsigned value)
  {
    TCHAR tmp[50];
    _sntprintf(tmp, 50, _T("%u"), value);
    return Set(key, tmp);
  }

  static inline bool Set(const TCHAR *key, bool value)
  {
    return Set(key, value ? _T("1") : _T("0"));
  }

  static inline bool Set(const TCHAR *key, int value)
  {
    TCHAR tmp[50];
    _sntprintf(tmp, 50, _T("%d"), value);
    return Set(key, tmp);
  }

  static inline bool Set(const TCHAR *key, long value)
  {
    TCHAR tmp[50];
    _sntprintf(tmp, 50, _T("%ld"), value);
    return Set(key, tmp);
  }

  static inline bool Set(const TCHAR *key, fixed value)
  {
    TCHAR tmp[50];
    _sntprintf(tmp, 50, _T("%f"), (double)value);
    return Set(key, tmp);
  }

  void Export(ProfileWriter &writer);
}

#endif
