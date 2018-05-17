/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_CONFIG_REGISTRY_HPP
#define XCSOAR_CONFIG_REGISTRY_HPP

#include <utility>

#include <windows.h>
#include <tchar.h>
#include <string.h>

/**
 * OO wrapper for a HKEY.
 */
class RegistryKey {
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

  RegistryKey(const RegistryKey &) = delete;
  RegistryKey &operator=(const RegistryKey &) = delete;

  RegistryKey(RegistryKey &&other):hKey(other.hKey) {
    other.hKey = 0;
  }

  RegistryKey &operator=(RegistryKey &&other) {
    std::swap(hKey, other.hKey);
    return *this;
  }

  bool error() const {
    return hKey == 0;
  }

  operator HKEY() const {
    return hKey;
  }

  bool GetValue(const TCHAR *name, LPDWORD type_r,
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
  bool GetValue(const TCHAR *name, TCHAR *value, unsigned max_length) const {
    DWORD type, length = max_length * sizeof(value[0]);
    return GetValue(name, &type, (LPBYTE)value, &length) && type == REG_SZ;
  }

  /**
   * Read an integer value.  The value is unchanged when this function
   * fails.
   *
   * @return true on success
   */
  bool GetValue(const TCHAR *name, DWORD &value_r) const {
    DWORD type, value, length = sizeof(value);
    if (!GetValue(name, &type, (LPBYTE)&value, &length) ||
        type != REG_DWORD || length != sizeof(value))
      return false;

    value_r = value;
    return true;
  }

  bool SetValue(const TCHAR *name, DWORD type,
                 const BYTE *data, DWORD length) {
    LONG result = ::RegSetValueEx(hKey, name, 0, type, data, length);
    return result == ERROR_SUCCESS;
  }

  bool SetValue(const TCHAR *name, const TCHAR *value) {
    return SetValue(name, REG_SZ, (const BYTE *)value,
                     (_tcslen(value) + 1) * sizeof(value[0]));
  }

  bool SetValue(const TCHAR *name, DWORD value) {
    return SetValue(name, REG_DWORD,
                     (const BYTE *)&value, sizeof(value));
  }

  bool DeleteValue(const TCHAR *name) {
    return ::RegDeleteValue(hKey, name) == ERROR_SUCCESS;
  }

  bool EnumKey(DWORD idx, TCHAR *name, size_t _name_max_length) const {
    DWORD name_max_length = (DWORD)_name_max_length;
    return ::RegEnumKeyEx(hKey, idx, name, &name_max_length,
                          NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
  }
};

#endif
