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

#include "Map.hpp"
#include "Util/NumberParser.hpp"

bool
ProfileMap::Get(const char *key, int &value) const
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a number
  char *endptr;
  int tmp = ParseInt(str, &endptr, 0);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

bool
ProfileMap::Get(const char *key, short &value) const
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a number
  char *endptr;
  short tmp = ParseInt(str, &endptr, 0);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

bool
ProfileMap::Get(const char *key, bool &value) const
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Save value to output parameter value and return success
  value = (str[0] != '0');
  return true;
}

bool
ProfileMap::Get(const char *key, unsigned &value) const
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a unsigned number
  char *endptr;
  unsigned tmp = ParseUnsigned(str, &endptr, 0);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

bool
ProfileMap::Get(const char *key, uint16_t &value) const
{
  unsigned value32;
  if (!Get(key, value32) || value32 >= 0x10000)
    return false;

  value = (uint16_t)value32;
  return true;
}

bool
ProfileMap::Get(const char *key, uint8_t &value) const
{
  unsigned value32;
  if (!Get(key, value32) || value32 >= 0x100)
    return false;

  value = (uint8_t)value32;
  return true;
}

bool
ProfileMap::Get(const char *key, double &value) const
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == nullptr)
    return false;

  // Parse the string for a floating point number
  char *endptr;
  double tmp = ParseDouble(str, &endptr);
  if (endptr == str)
    return false;

  // Save parsed value to output parameter value and return success
  value = tmp;
  return true;
}

void
ProfileMap::Set(const char *key, int value)
{
  char tmp[50];
  sprintf(tmp, "%d", value);
  Set(key, tmp);
}

void
ProfileMap::Set(const char *key, long value)
{
  char tmp[50];
  sprintf(tmp, "%ld", value);
  Set(key, tmp);
}

void
ProfileMap::Set(const char *key, unsigned value)
{
  char tmp[50];
  sprintf(tmp, "%u", value);
  Set(key, tmp);
}

void
ProfileMap::Set(const char *key, double value)
{
  char tmp[50];
  sprintf(tmp, "%f", value);
  Set(key, tmp);
}
