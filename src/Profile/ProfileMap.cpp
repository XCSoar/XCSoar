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

#include "ProfileMap.hpp"
#include "Current.hpp"
#include "Map.hpp"

bool
Profile::IsModified()
{
  return map.IsModified();
}

void
Profile::SetModified(bool _modified)
{
  map.SetModified(_modified);
}

const char *
Profile::Get(const char *key, const char *default_value)
{
  return map.Get(key, default_value);
}

bool
Profile::Get(const char *key, TCHAR *value, size_t max_size)
{
  return map.Get(key, value, max_size);
}

bool
Profile::Get(const char *key, int &value)
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, short &value)
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, bool &value)
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, unsigned &value)
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, uint16_t &value)
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, uint8_t &value)
{
  return map.Get(key, value);
}

bool
Profile::Get(const char *key, double &value)
{
  return map.Get(key, value);
}

void
Profile::Set(const char *key, const char *value)
{
  map.Set(key, value);
}

#ifdef _UNICODE

void
Profile::Set(const char *key, const TCHAR *value)
{
  map.Set(key, value);
}

#endif

void
Profile::Set(const char *key, int value)
{
  map.Set(key, value);
}

void
Profile::Set(const char *key, long value)
{
  map.Set(key, value);
}

void
Profile::Set(const char *key, unsigned value)
{
  map.Set(key, value);
}

void
Profile::Set(const char *key, double value)
{
  map.Set(key, value);
}

bool
Profile::Exists(const char *key)
{
  return map.Exists(key);
}

void
Profile::Clear()
{
  map.clear();
}
