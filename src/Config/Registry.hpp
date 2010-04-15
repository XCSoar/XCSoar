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

#ifndef XCSOAR_CONFIG_REGISTRY_HPP
#define XCSOAR_CONFIG_REGISTRY_HPP

#include "Util/NonCopyable.hpp"

#include <windows.h>
#include <tchar.h>
#include <string.h>

/**
 * OO wrapper for a HKEY.
 */
class RegistryKey : private NonCopyable {
protected:
  HKEY hKey;

public:
  RegistryKey(HKEY hParent, const TCHAR *key, bool read_only) {
    DWORD disposition;
    LONG result = read_only
      ? ::RegOpenKeyEx(hParent, key, 0, KEY_READ, &hKey)
      : ::RegCreateKeyEx(hParent, key, 0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_WRITE, NULL,
                         &hKey, &disposition);
    if (result != ERROR_SUCCESS)
      hKey = 0;
  }

  ~RegistryKey() {
    if (hKey != 0)
      ::RegCloseKey(hKey);
  }

  bool error() const {
    return hKey == 0;
  }

  bool get_value(const TCHAR *name, LPDWORD type_r,
                 LPBYTE data, LPDWORD length_r) const {
    LONG result = ::RegQueryValueEx(hKey, name, NULL, type_r, data, length_r);
    return result == ERROR_SUCCESS;
  }

  /**
   * Reads a string value.  When this function fails, the value in the
   * buffer is undefined (may have been modified by this method).
   *
   * @return true on success
   */
  bool get_value(const TCHAR *name, TCHAR *value, unsigned max_length) const {
    DWORD type, length = max_length * sizeof(value[0]);
    return get_value(name, &type, (LPBYTE)value, &length) && type == REG_SZ;
  }

  /**
   * Read an integer value.  The value is unchanged when this function
   * fails.
   *
   * @return true on success
   */
  bool get_value(const TCHAR *name, DWORD &value_r) const {
    DWORD type, value, length = sizeof(value);
    if (!get_value(name, &type, (LPBYTE)&value, &length) ||
        type != REG_DWORD || length != sizeof(value))
      return false;

    value_r = value;
    return true;
  }

  bool set_value(const TCHAR *name, DWORD type,
                 const BYTE *data, DWORD length) {
    LONG result = ::RegSetValueEx(hKey, name, 0, type, data, length);
    return result == ERROR_SUCCESS;
  }

  bool set_value(const TCHAR *name, const TCHAR *value) {
    return set_value(name, REG_SZ, (const BYTE *)value,
                     (_tcslen(value) + 1) * sizeof(value[0]));
  }

  bool set_value(const TCHAR *name, DWORD value) {
    return set_value(name, REG_DWORD,
                     (const BYTE *)&value, sizeof(value));
  }
};

#endif
