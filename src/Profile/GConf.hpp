/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include <windef.h>
#include <tchar.h>

class ProfileWriter;

namespace ProfileGConf {
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

    value = temp;
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

  bool Get(const TCHAR *szRegValue, TCHAR *pPos, size_t dwSize);
  bool Set(const TCHAR *szRegValue, const TCHAR *Pos);

  void Export(ProfileWriter &writer);
}

#endif
