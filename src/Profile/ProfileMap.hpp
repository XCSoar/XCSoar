/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "util/StringBuffer.hxx"

#include <chrono>
#include <cstdint>

#include <tchar.h>

class KeyValueFileWriter;

namespace Profile {

/**
 * Has the in-memory profile been modified since the last
 * SetModified(false) call?
 */
[[gnu::pure]]
bool
IsModified() noexcept;

/**
 * Set the "modified" flag.
 */
void
SetModified(bool modified) noexcept;

/**
 * Look up a string value in the profile.
 *
 * @param key name of the value
 * @param default_value a value to be returned when the key does not exist
 * @return the value (gets Invalidated by any write access to the
 * profile), or default_value if the key does not exist
 */
[[gnu::pure]]
const char *
Get(const char *key, const char *default_value=nullptr) noexcept;

/**
 * Reads a value from the profile map
 * @param key Name of the value that should be read
 * @param value Pointer to the output buffer
 * @param max_size Maximum size of the output buffer
 */
bool
Get(const char *key, TCHAR *value, std::size_t max_size) noexcept;

/**
 * Writes a value to the profile map
 * @param key Name of the value that should be written
 * @param value Value that should be written
 */
void
Set(const char *key, const char *value) noexcept;

#ifdef _UNICODE
void
Set(const char *key, const TCHAR *value) noexcept;
#endif

bool
Get(const char *key, int &value) noexcept;

bool
Get(const char *key, short &value) noexcept;

bool
Get(const char *key, bool &value) noexcept;

bool
Get(const char *key, unsigned &value) noexcept;

bool
Get(const char *key, uint16_t &value) noexcept;

bool
Get(const char *key, uint8_t &value) noexcept;

bool
Get(const char *key, double &value) noexcept;

template<typename T>
static inline bool
GetEnum(const char *key, T &value) noexcept
{
  int i;
  if (Get(key, i)) {
    value = (T)i;
    return true;
  } else
    return false;
}

static inline void
Set(const char *key, bool value) noexcept
{
  Set(key, value ? _T("1") : _T("0"));
}

void
Set(const char *key, int value) noexcept;

void
Set(const char *key, long value) noexcept;

void
Set(const char *key, unsigned value) noexcept;

void
Set(const char *key, double value) noexcept;

static inline void
Set(const char *key, std::chrono::duration<unsigned> value) noexcept
{
  Set(key, value.count());
}

template<typename T>
static inline void
SetEnum(const char *key, T value) noexcept
{
  Set(key, (int)value);
}

template<std::size_t max>
static inline bool
Get(const char *key, BasicStringBuffer<TCHAR, max> &value) noexcept
{
  return Get(key, value.data(), value.capacity());
}

bool
Exists(const char *key) noexcept;

void
Clear() noexcept;

} // namespace Profile
