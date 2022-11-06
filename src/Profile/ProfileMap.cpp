/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
