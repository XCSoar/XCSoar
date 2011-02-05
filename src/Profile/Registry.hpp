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

#ifndef XCSOAR_REGISTRY_HPP
#define XCSOAR_REGISTRY_HPP

#include "Util/StaticString.hpp"
#include "Math/fixed.hpp"

#include <windef.h>
#include <tchar.h>

class ProfileWriter;

namespace Registry {
  bool Get(const TCHAR *szRegValue, DWORD &pPos);

  static inline bool Get(const TCHAR *key, int &value)
  {
    DWORD temp;
    if (!Get(key, temp))
      return false;

    value = temp;
    return true;
  }

  static inline bool Get(const TCHAR *key, short &value)
  {
    DWORD temp;
    if (!Get(key, temp))
      return false;

    value = (short)temp;
    return true;
  }

  static inline bool Get(const TCHAR *key, bool &value)
  {
    DWORD temp;
    if (!Get(key, temp))
      return false;

    value = temp > 0;
    return true;
  }

#if !defined(HAVE_POSIX) || defined(__CYGWIN__) /* DWORD==unsigned on WINE, would be duplicate */
  static inline bool Get(const TCHAR *key, unsigned &value)
  {
    DWORD temp;
    if (!Get(key, temp))
      return false;

    value = temp;
    return true;
  }
#endif

  static inline bool Get(const TCHAR *key, fixed &value)
  {
    DWORD temp;
    if (!Get(key, temp))
      return false;

    value = fixed(temp);
    return true;
  }

  bool Set(const TCHAR *szRegValue, DWORD Pos);

  static inline bool Set(const TCHAR *key, bool value)
  {
    return Set(key, value ? DWORD(1) : DWORD(0));
  }

  static inline bool Set(const TCHAR *key, int value)
  {
    return Set(key, DWORD(value));
  }

  static inline bool Set(const TCHAR *key, long value)
  {
    return Set(key, DWORD(value));
  }

  #if !defined(HAVE_POSIX) || defined(__CYGWIN__) /* DWORD==unsigned on WINE, would be duplicate */
  static inline bool Set(const TCHAR *key, unsigned value)
  {
    return Set(key, DWORD(value));
  }
  #endif

  bool Get(const TCHAR *szRegValue, TCHAR *pPos, size_t dwSize);
  bool Set(const TCHAR *szRegValue, const TCHAR *Pos);

  template<unsigned max>
  static inline bool
  Get(const TCHAR *name, StaticString<max> &value)
  {
    return Get(name, value.buffer(), value.MAX_SIZE);
  }

  void Export(ProfileWriter &writer);
}

#endif
