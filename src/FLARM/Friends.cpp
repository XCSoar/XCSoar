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
#include "Profile/Profile.hpp"
#include "Util/tstring.hpp"

#include <map>

namespace FlarmFriends
{
  bool loaded = false;
  std::map<FlarmId, Color> friends;

  void LoadColor(const TCHAR *key, Color color);
}

FlarmFriends::Color
FlarmFriends::GetFriendColor(FlarmId id)
{
  auto i = friends.find(id);
  if (i != friends.end())
    return i->second;

  return Color::NONE;
}

void
FlarmFriends::SetFriendColor(FlarmId id, Color color)
{
  friends[id] = color;
}

void
FlarmFriends::LoadColor(const TCHAR *key, Color color)
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
  LoadColor(_T("FriendsGreen"), Color::GREEN);
  LoadColor(_T("FriendsBlue"), Color::BLUE);
  LoadColor(_T("FriendsYellow"), Color::YELLOW);
  LoadColor(_T("FriendsMagenta"), Color::MAGENTA);

  loaded = true;
}

void
FlarmFriends::Save()
{
  if (!loaded)
    return;

  TCHAR id[16];
  tstring ids[4];

  for (const auto &i : friends) {
    assert(i.first.IsDefined());
    assert((int)i.second < (int)Color::COUNT);

    if (i.second == Color::NONE)
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
