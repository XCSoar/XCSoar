// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProfileMap.hpp"
#include "Current.hpp"
#include "Map.hpp"

bool
Profile::IsModified() noexcept
{
  return map.IsModified();
}

void
Profile::SetModified(bool _modified) noexcept
{
  map.SetModified(_modified);
}

const char *
Profile::Get(const char *key, const char *default_value) noexcept
{
  return map.Get(key, default_value);
}

bool
Profile::Get(const char *key, TCHAR *value, std::size_t max_size) noexcept
{
  return map.Get(key, value, max_size);
}

bool
Profile::Get(const char *key, int &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, short &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, bool &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, unsigned &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, uint16_t &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, uint8_t &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, double &value) noexcept
{
  return map.Get(key, value);
}

void
Profile::Set(const char *key, const char *value) noexcept
{
  map.Set(key, value);
}

#ifdef _UNICODE

void
Profile::Set(const char *key, const TCHAR *value) noexcept
{
  map.Set(key, value);
}

#endif

void
Profile::Set(const char *key, int value) noexcept
{
  map.Set(key, value);
}

void
Profile::Set(const char *key, long value) noexcept
{
  map.Set(key, value);
}

void
Profile::Set(const char *key, unsigned value) noexcept
{
  map.Set(key, value);
}

void
Profile::Set(const char *key, double value) noexcept
{
  map.Set(key, value);
}

bool
Profile::Exists(const char *key) noexcept
{
  return map.Exists(key);
}

void
Profile::Clear() noexcept
{
  map.Clear();
}
