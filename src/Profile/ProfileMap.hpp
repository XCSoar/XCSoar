// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
