// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/FloatDuration.hxx"
#include "util/StringBuffer.hxx"

#include <map>
#include <string>

#include <cstdint>
#include <tchar.h>

struct GeoPoint;
class RGB8Color;
class Path;
class AllocatedPath;
template<typename T> class StringPointer;
template<typename T> class BasicAllocatedString;

class ProfileMap {
  std::map<std::string, std::string, std::less<>> map;

  bool modified = false;

public:
  /**
   * Has the profile been modified since the last SetModified(false)
   * call?
   */
  bool IsModified() const noexcept {
    return modified;
  }

  /**
   * Set the "modified" flag.
   */
  void SetModified(bool _modified=true) noexcept {
    modified = _modified;
  }

  void Clear() noexcept {
    map.clear();
  }

  [[gnu::pure]]
  bool Exists(const char *key) const noexcept {
    return map.find(key) != map.end();
  }

  void Remove(const char *key) noexcept {
    if (auto i = map.find(key); i != map.end())
      map.erase(i);
  }

  [[gnu::pure]]
  auto begin() const noexcept {
    return map.begin();
  }

  [[gnu::pure]]
  auto end() const noexcept {
    return map.end();
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
  [[gnu::pure]]
  const char *Get(const char *key,
                  const char *default_value=nullptr) const noexcept {
    const auto i = map.find(key);
    if (i == map.end())
      return default_value;

    return i->second.c_str();
  }

  void Set(const char *key, const char *value) noexcept;

  // TCHAR string values

  /**
   * Reads a value from the profile map
   *
   * @param key name of the value that should be read
   * @param value Pointer to the output buffer
   * @param max_size maximum size of the output buffer
   */
  bool Get(const char *key, TCHAR *value, size_t max_size) const noexcept;

  template<size_t max>
  bool Get(const char *key,
           BasicStringBuffer<TCHAR, max> &value) const noexcept {
    return Get(key, value.data(), value.capacity());
  }

#ifdef _UNICODE
  void Set(const char *key, const TCHAR *value) noexcept;
#endif

  // numeric values

  bool Get(const char *key, int &value) const noexcept;
  bool Get(const char *key, short &value) const noexcept;
  bool Get(const char *key, bool &value) const noexcept;
  bool Get(const char *key, unsigned &value) const noexcept;
  bool Get(const char *key, uint16_t &value) const noexcept;
  bool Get(const char *key, uint8_t &value) const noexcept;
  bool Get(const char *key, double &value) const noexcept;

  bool Get(const char *key, FloatDuration &value) const noexcept {
    double _value;
    bool result = Get(key, _value);
    if (result)
      value = FloatDuration{_value};
    return result;
  }

  bool Get(const char *key,
           std::chrono::duration<unsigned> &value) const noexcept {
    unsigned _value;
    bool result = Get(key, _value);
    if (result)
      value = std::chrono::duration<unsigned>{_value};
    return result;
  }

  void Set(const char *key, bool value) noexcept {
    Set(key, value ? "1" : "0");
  }

  void Set(const char *key, int value) noexcept;
  void Set(const char *key, long value) noexcept;
  void Set(const char *key, unsigned value) noexcept;
  void Set(const char *key, double value) noexcept;

  void Set(const char *key, FloatDuration value) noexcept {
    Set(key, value.count());
  }

  void Set(const char *key, std::chrono::duration<unsigned> value) noexcept {
    Set(key, value.count());
  }

  // enum values

  template<typename T>
  bool GetEnum(const char *key, T &value) const noexcept {
    int i;
    bool success = Get(key, i);
    if (success)
      value = T(i);
    return success;
  }

  template<typename T>
  void SetEnum(const char *key, T value) noexcept {
    Set(key, (int)value);
  }

  // path values

  AllocatedPath GetPath(const char *key) const noexcept;

  [[gnu::pure]]
  bool GetPathIsEqual(const char *key, Path value) const noexcept;

  /**
   * Gets a path from the profile and return its base name only.
   */
#ifdef _UNICODE
  BasicAllocatedString<TCHAR> GetPathBase(const char *key) const noexcept;
#else
  [[gnu::pure]]
  StringPointer<TCHAR> GetPathBase(const char *key) const noexcept;
#endif

  void SetPath(const char *key, Path value) noexcept;

  // geo value

  /**
   * Load a GeoPoint from the profile.
   */
  bool GetGeoPoint(const char *key, GeoPoint &value) const noexcept;

  /**
   * Save a GeoPoint to the profile.  It is stored as a string,
   * longitude and latitude formatted in degrees separated by a space
   * character.
   */
  void SetGeoPoint(const char *key, const GeoPoint &value) noexcept;

  // screen values

  /**
   * Load a Color from the profile.
   */
  bool GetColor(const char *key, RGB8Color &value) const noexcept;

  /**
   * Save a Color to the profile.  It is stored as a RGB hex string
   * e.g. #123456
   */
  void SetColor(const char *key, const RGB8Color value) noexcept;
};
