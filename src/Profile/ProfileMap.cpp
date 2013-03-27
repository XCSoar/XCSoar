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

#include "Profile/ProfileMap.hpp"
#include "IO/KeyValueFileWriter.hpp"
#include "Util/StringUtil.hpp"
#include "Util/NumberParser.hpp"
#include "Util/Macros.hpp"

#include <map>
#include <string>

#ifdef _UNICODE
#include <windows.h>
#endif

namespace ProfileMap {
  typedef std::map<std::string, std::string> map_t;

  static map_t map;
  static bool modified;
}

bool
ProfileMap::IsModified()
{
  return modified;
}

void
ProfileMap::SetModified(bool _modified)
{
  modified = _modified;
}

const char *
ProfileMap::Get(const char *key, const char *default_value)
{
  map_t::const_iterator it = map.find(key);
  if (it == map.end())
    return default_value;

  return it->second.c_str();
}

bool
ProfileMap::Get(const char *key, TCHAR *value, size_t max_size)
{
  map_t::const_iterator it = map.find(key);
  if (it == map.end()) {
    value[0] = _T('\0');
    return false;
  }

  const char *src = it->second.c_str();

#ifdef _UNICODE
  int result = MultiByteToWideChar(CP_UTF8, 0, src, -1,
                                   value, max_size);
  return result > 0;
#else
  if (!ValidateUTF8(src))
    return false;

  CopyString(value, src, max_size);
  return true;
#endif
}

bool
ProfileMap::Get(const char *key, int &value)
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == NULL)
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
ProfileMap::Get(const char *key, short &value)
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == NULL)
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
ProfileMap::Get(const char *key, bool &value)
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == NULL)
    return false;

  // Save value to output parameter value and return success
  value = (str[0] != '0');
  return true;
}

bool
ProfileMap::Get(const char *key, unsigned &value)
{
  // Try to read the profile map
  const char *str = Get(key);
  if (str == NULL)
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
ProfileMap::Get(const char *key, uint16_t &value)
{
  unsigned value32;
  if (!Get(key, value32) || value32 >= 0x10000)
    return false;

  value = (uint16_t)value32;
  return true;
}

bool
ProfileMap::Get(const char *key, uint8_t &value)
{
  unsigned value32;
  if (!Get(key, value32) || value32 >= 0x100)
    return false;

  value = (uint8_t)value32;
  return true;
}

bool
ProfileMap::Get(const char *key, fixed &value)
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
  value = fixed(tmp);
  return true;
}

void
ProfileMap::Set(const char *key, const char *value)
{
  auto i = map.insert(std::make_pair(key, value));
  if (!i.second) {
    /* exists already */

    if (i.first->second.compare(value) == 0)
      /* not modified, don't set the "modified" flag */
      return;

    i.first->second.assign(value);
  }

  modified = true;
}

#ifdef _UNICODE

void
ProfileMap::Set(const char *key, const TCHAR *value)
{
  char buffer[MAX_PATH];
  int length = WideCharToMultiByte(CP_UTF8, 0, value, -1,
                                   buffer, ARRAY_SIZE(buffer),
                                   nullptr, nullptr);
  if (length <= 0)
    return;

  Set(key, buffer);
}

#endif

void
ProfileMap::Set(const char *key, int value)
{
  char tmp[50];
  sprintf(tmp, "%d", value);
  return Set(key, tmp);
}

void
ProfileMap::Set(const char *key, long value)
{
  char tmp[50];
  sprintf(tmp, "%ld", value);
  return Set(key, tmp);
}

void
ProfileMap::Set(const char *key, unsigned value)
{
  char tmp[50];
  sprintf(tmp, "%u", value);
  return Set(key, tmp);
}

void
ProfileMap::Set(const char *key, fixed value)
{
  char tmp[50];
  sprintf(tmp, "%f", (double)value);
  return Set(key, tmp);
}

bool
ProfileMap::Exists(const char *key)
{
  return map.find(key) != map.end();
}

void
ProfileMap::Export(KeyValueFileWriter &writer)
{
  // Iterate through the profile maps
  for (auto it_str = map.begin(); it_str != map.end(); it_str++)
    /* ignore the "Vega*" values; the Vega driver abuses the profile
       to pass messages between the driver and the user interface */
    if (strncmp(it_str->first.c_str(), "Vega", 4) != 0)
      writer.Write(it_str->first.c_str(), it_str->second.c_str());
}

void
ProfileMap::Clear()
{
  map.clear();
}
