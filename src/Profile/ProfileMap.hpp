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

#ifndef XCSOAR_PROFILE_MAP_HPP
#define XCSOAR_PROFILE_MAP_HPP

#include "Util/StaticString.hpp"
#include "Math/fixed.hpp"

#include <tchar.h>
#include <stdint.h>

class KeyValueFileWriter;

namespace ProfileMap {
  /**
   * Has the in-memory profile been modified since the last
   * SetModified(false) call?
   */
  gcc_pure
  bool IsModified();

  /**
   * Set the "modified" flag.
   */
  void SetModified(bool modified);

  /**
   * Look up a string value in the profile.
   *
   * @param key name of the value
   * @param default_value a value to be returned when the key does not exist
   * @return the value (gets Invalidated by any write access to the
   * profile), or default_value if the key does not exist
   */
  gcc_pure
  const TCHAR *Get(const TCHAR *key, const TCHAR *default_value=NULL);

  /**
   * Reads a value from the profile map
   * @param key Name of the value that should be read
   * @param value Pointer to the output buffer
   * @param max_size Maximum size of the output buffer
   */
  bool Get(const TCHAR *key, TCHAR *value, size_t max_size);

  /**
   * Writes a value to the profile map
   * @param key Name of the value that should be written
   * @param value Value that should be written
   */
  void Set(const TCHAR *key, const TCHAR *value);

  bool Get(const TCHAR *key, int &value);
  bool Get(const TCHAR *key, short &value);
  bool Get(const TCHAR *key, bool &value);
  bool Get(const TCHAR *key, unsigned &value);
  bool Get(const TCHAR *key, uint16_t &value);
  bool Get(const TCHAR *key, uint8_t &value);
  bool Get(const TCHAR *key, fixed &value);

  template<typename T>
  static inline bool GetEnum(const TCHAR *key, T &value)
  {
    int i;
    if (Get(key, i)) {
      value = (T)i;
      return true;
    } else
      return false;
  }

  static inline void Set(const TCHAR *key, bool value)
  {
    Set(key, value ? _T("1") : _T("0"));
  }

  void Set(const TCHAR *key, int value);
  void Set(const TCHAR *key, long value);
  void Set(const TCHAR *key, unsigned value);
  void Set(const TCHAR *key, fixed value);

  template<typename T>
  static inline void SetEnum(const TCHAR *key, T value)
  {
    Set(key, (int)value);
  }

  template<size_t max>
  static inline bool
  Get(const TCHAR *key, StaticString<max> &value)
  {
    return Get(key, value.buffer(), value.MAX_SIZE);
  }

  bool Exists(const TCHAR *key);
  void Clear();

  void Export(KeyValueFileWriter &writer);
}

#endif
