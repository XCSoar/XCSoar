/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Util/tstring.hpp"
#include "StringUtil.hpp"

#include <map>

namespace ProfileMap {
  typedef std::map<tstring, tstring> map_t;

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

const TCHAR *
ProfileMap::Get(const TCHAR *key, const TCHAR *default_value)
{
  map_t::const_iterator it = map.find(key);
  if (it == map.end())
    return default_value;

  return it->second.c_str();
}

bool
ProfileMap::Get(const TCHAR *key, TCHAR *value, size_t max_size)
{
  map_t::const_iterator it = map.find(key);
  if (it == map.end()) {
    value[0] = _T('\0');
    return false;
  }

  const TCHAR *src = it->second.c_str();
  CopyString(value, src, max_size);
  return true;
}

bool
ProfileMap::Set(const TCHAR *key, const TCHAR *value)
{
  map[key] = value;
  modified = true;
  return true;
}

bool
ProfileMap::Exists(const TCHAR *key)
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
    if (_tcsncmp(it_str->first.c_str(), _T("Vega"), 4) != 0)
      writer.Write(it_str->first.c_str(), it_str->second.c_str());
}

void
ProfileMap::Clear()
{
  map.clear();
}
