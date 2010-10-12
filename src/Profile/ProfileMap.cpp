/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
  typedef std::map<tstring, tstring> map_str_t;
  typedef std::map<tstring, DWORD> map_num_t;

  static map_str_t map_str;
  static map_num_t map_num;
}

bool
ProfileMap::Get(const TCHAR *szRegValue, DWORD &pPos)
{
  map_num_t::const_iterator it = map_num.find(szRegValue);
  if (it == map_num.end())
    return false;

  pPos = it->second;
  return true;
}

bool
ProfileMap::Set(const TCHAR *szRegValue, DWORD Pos)
{
  map_num[szRegValue] = Pos;
  return true;
}

/**
 * Reads a value from the profile map
 * @param szRegValue Name of the value that should be read
 * @param pPos Pointer to the output buffer
 * @param dwSize Maximum size of the output buffer
 */
bool
ProfileMap::Get(const TCHAR *szRegValue, TCHAR *pPos, size_t dwSize)
{
  map_str_t::const_iterator it = map_str.find(szRegValue);
  if (it == map_str.end()) {
    pPos[0] = _T('\0');
    return false;
  }

  _tcsncpy(pPos, it->second.c_str(), dwSize);
  return true;
}

/**
 * Writes a value to the profile map
 * @param szRegValue Name of the value that should be written
 * @param Pos Value that should be written
 */
bool
ProfileMap::Set(const TCHAR *szRegValue, const TCHAR *Pos)
{
  map_str[szRegValue] = Pos;
  return true;
}

void
ProfileMap::Export(ProfileWriter &writer)
{
  // Iterate through the profile maps
  for (map_num_t::const_iterator it_num = map_num.begin();
       it_num != map_num.end(); it_num++)
    writer.write(it_num->first.c_str(), it_num->second);

  for (map_str_t::const_iterator it_str = map_str.begin();
       it_str != map_str.end(); it_str++)
    writer.write(it_str->first.c_str(), it_str->second.c_str());
}
