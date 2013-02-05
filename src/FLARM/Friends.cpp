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

#include "FLARM/Friends.hpp"
#include "FLARM/FlarmId.hpp"
#include "ColorDatabase.hpp"
#include "Profile/Profile.hpp"
#include "Util/tstring.hpp"

#include <map>

namespace FlarmFriends
{
  static FlarmColorDatabase *database;

  void LoadColor(const TCHAR *key, FlarmColor color);
}

FlarmColor
FlarmFriends::GetFriendColor(FlarmId id)
{
  assert(database != nullptr);

  return database->Get(id);
}

void
FlarmFriends::SetFriendColor(FlarmId id, FlarmColor color)
{
  assert(database != nullptr);

  database->Set(id, color);
}

void
FlarmFriends::LoadColor(const TCHAR *key, FlarmColor color)
{
  const TCHAR *ids = Profile::Get(key);
  if (ids == nullptr)
    return;

  const TCHAR *p = ids;
  while (true) {
    TCHAR *endptr;
    FlarmId id = FlarmId::Parse(p, &endptr);
    if (id.IsDefined())
      SetFriendColor(id, color);

    p = _tcschr(endptr, _T(','));
    if (p == nullptr)
      break;

    ++p;
  }
}

void
FlarmFriends::Load()
{
  assert(database == nullptr);

  database = new FlarmColorDatabase();

  LoadColor(_T("FriendsGreen"), FlarmColor::GREEN);
  LoadColor(_T("FriendsBlue"), FlarmColor::BLUE);
  LoadColor(_T("FriendsYellow"), FlarmColor::YELLOW);
  LoadColor(_T("FriendsMagenta"), FlarmColor::MAGENTA);
}

void
FlarmFriends::Save()
{
  if (database == nullptr)
    return;

  TCHAR id[16];
  tstring ids[4];

  for (const auto &i : *database) {
    assert(i.first.IsDefined());
    assert((int)i.second < (int)FlarmColor::COUNT);

    if (i.second == FlarmColor::NONE)
      continue;

    unsigned color_index = (int)i.second - 1;

    i.first.Format(id);
    ids[color_index] += id;
    ids[color_index] += ',';
  }

  Profile::Set(_T("FriendsGreen"), ids[0].c_str());
  Profile::Set(_T("FriendsBlue"), ids[1].c_str());
  Profile::Set(_T("FriendsYellow"), ids[2].c_str());
  Profile::Set(_T("FriendsMagenta"), ids[3].c_str());
}

void
FlarmFriends::Destroy()
{
  delete database;
}
