/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Profile/Writer.hpp"
#include "Util/tstring.hpp"
#include "StringUtil.hpp"

#include <map>

namespace ProfileMap {
  typedef std::map<tstring, tstring> map_t;

  static map_t map;
}

bool
ProfileMap::Get(const TCHAR *szRegValue, TCHAR *pPos, size_t dwSize)
{
  map_t::const_iterator it = map.find(szRegValue);
  if (it == map.end()) {
    pPos[0] = _T('\0');
    return false;
  }

  _tcsncpy(pPos, it->second.c_str(), dwSize);
  return true;
}

bool
ProfileMap::Set(const TCHAR *szRegValue, const TCHAR *Pos)
{
  map[szRegValue] = Pos;
  return true;
}

void
ProfileMap::Export(ProfileWriter &writer)
{
  // Iterate through the profile maps
  for (map_t::const_iterator it_str = map.begin();
       it_str != map.end(); it_str++)
    writer.write(it_str->first.c_str(), it_str->second.c_str());
}
