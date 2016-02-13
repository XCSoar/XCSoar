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

#ifndef XCSOAR_PROFILE_MAP2_HPP
#define XCSOAR_PROFILE_MAP2_HPP

#include "Util/StringBuffer.hxx"
#include "Compiler.h"

#include <map>
#include <string>

#include <stdint.h>
#include <tchar.h>

struct GeoPoint;
class RGB8Color;
class Path;
class AllocatedPath;
template<typename T> class StringPointer;
template<typename T> class AllocatedString;

class ProfileMap : public std::map<std::string, std::string> {
  bool modified;

public:
  ProfileMap():modified(false) {}

  /**
   * Has the profile been modified since the last SetModified(false)
   * call?
   */
  bool IsModified() const {
    return modified;
  }

  /**
   * Set the "modified" flag.
   */
  void SetModified(bool _modified=true) {
    modified = _modified;
  }

  gcc_pure
  bool Exists(const char *key) const {
    return find(key) != end();
  }

  // basic string values

  /**
   * Look up a string value in the profile.
   *
   * @param key name of the value
   * @param default_value a value to be returned when the key does not exist
   * @return the value (gets Invalidated by any write access to the
   * profile), or default_value if the key does not exist
   */
  gcc_pure
  const char *Get(const char *key, const char *default_value=nullptr) const {
    const auto i = find(key);
    if (i == end())
      return default_value;

    return i->second.c_str();
  }

  void Set(const char *key, const char *value);

  // TCHAR string values

  /**
   * Reads a value from the profile map
   *
   * @param key name of the value that should be read
   * @param value Pointer to the output buffer
   * @param max_size maximum size of the output buffer
   */
  bool Get(const char *key, TCHAR *value, size_t max_size) const;

  template<size_t max>
  bool Get(const char *key, StringBuffer<TCHAR, max> &value) const {
    return Get(key, value.data(), value.capacity());
  }

#ifdef _UNICODE
  void Set(const char *key, const TCHAR *value);
#endif

  // numeric values

  bool Get(const char *key, int &value) const;
  bool Get(const char *key, short &value) const;
  bool Get(const char *key, bool &value) const;
  bool Get(const char *key, unsigned &value) const;
  bool Get(const char *key, uint16_t &value) const;
  bool Get(const char *key, uint8_t &value) const;
  bool Get(const char *key, double &value) const;

  void Set(const char *key, bool value) {
    Set(key, value ? "1" : "0");
  }

  void Set(const char *key, int value);
  void Set(const char *key, long value);
  void Set(const char *key, unsigned value);
  void Set(const char *key, double value);

  // enum values

  template<typename T>
  bool GetEnum(const char *key, T &value) const {
    int i;
    bool success = Get(key, i);
    if (success)
      value = T(i);
    return success;
  }

  template<typename T>
  void SetEnum(const char *key, T value) {
    Set(key, (int)value);
  }

  // path values

  AllocatedPath GetPath(const char *key) const;

  gcc_pure
  bool GetPathIsEqual(const char *key, Path value) const;

  /**
   * Gets a path from the profile and return its base name only.
   */
#ifdef _UNICODE
  AllocatedString<TCHAR> GetPathBase(const char *key) const;
#else
  gcc_pure
  StringPointer<TCHAR> GetPathBase(const char *key) const;
#endif

  void SetPath(const char *key, Path value);

  // geo value

  /**
   * Load a GeoPoint from the profile.
   */
  bool GetGeoPoint(const char *key, GeoPoint &value) const;

  /**
   * Save a GeoPoint to the profile.  It is stored as a string,
   * longitude and latitude formatted in degrees separated by a space
   * character.
   */
  void SetGeoPoint(const char *key, const GeoPoint &value);

  // screen values

  /**
   * Load a Color from the profile.
   */
  bool GetColor(const char *key, RGB8Color &value) const;

  /**
   * Save a Color to the profile.  It is stored as a RGB hex string
   * e.g. #123456
   */
  void SetColor(const char *key, const RGB8Color value);
};

#endif
