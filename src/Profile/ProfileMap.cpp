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
Profile::Get(std::string_view key, const char *default_value) noexcept
{
  return map.Get(key, default_value);
}

bool
Profile::Get(std::string_view key, std::span<TCHAR> value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(std::string_view key, int &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(std::string_view key, short &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(std::string_view key, bool &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(std::string_view key, unsigned &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(std::string_view key, uint16_t &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(std::string_view key, uint8_t &value) noexcept
{
  return map.Get(key, value);
}

bool
Profile::Get(std::string_view key, double &value) noexcept
{
  return map.Get(key, value);
}

void
Profile::Set(std::string_view key, const char *value) noexcept
{
  map.Set(key, value);
}

#ifdef _UNICODE

void
Profile::Set(std::string_view key, const TCHAR *value) noexcept
{
  map.Set(key, value);
}

#endif

void
Profile::Set(std::string_view key, int value) noexcept
{
  map.Set(key, value);
}

void
Profile::Set(std::string_view key, long value) noexcept
{
  map.Set(key, value);
}

void
Profile::Set(std::string_view key, unsigned value) noexcept
{
  map.Set(key, value);
}

void
Profile::Set(std::string_view key, double value) noexcept
{
  map.Set(key, value);
}

bool
Profile::Exists(std::string_view key) noexcept
{
  return map.Exists(key);
}

void
Profile::Clear() noexcept
{
  map.Clear();
}
