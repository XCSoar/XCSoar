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

#include "FlarmProfile.hpp"
#include "Profile.hpp"
#include "FLARM/ColorDatabase.hpp"

#include <string>

static void
LoadColor(FlarmColorDatabase &db, const char *key, FlarmColor color)
{
  const char *ids = Profile::Get(key);
  if (ids == nullptr)
    return;

  const char *p = ids;
  while (true) {
    char *endptr;
    FlarmId id = FlarmId::Parse(p, &endptr);
    if (id.IsDefined())
      db.Set(id, color);

    p = strchr(endptr, ',');
    if (p == nullptr)
      break;

    ++p;
  }
}

void
Profile::Load(FlarmColorDatabase &db)
{
  LoadColor(db, "FriendsGreen", FlarmColor::GREEN);
  LoadColor(db, "FriendsBlue", FlarmColor::BLUE);
  LoadColor(db, "FriendsYellow", FlarmColor::YELLOW);
  LoadColor(db, "FriendsMagenta", FlarmColor::MAGENTA);
}

void
Profile::Save(const FlarmColorDatabase &db)
{
  char id[16];
  std::string ids[4];

  for (const auto &i : db) {
    assert(i.first.IsDefined());
    assert((int)i.second < (int)FlarmColor::COUNT);

    if (i.second == FlarmColor::NONE)
      continue;

    unsigned color_index = (int)i.second - 1;

    i.first.Format(id);
    ids[color_index] += id;
    ids[color_index] += ',';
  }

  Set("FriendsGreen", ids[0].c_str());
  Set("FriendsBlue", ids[1].c_str());
  Set("FriendsYellow", ids[2].c_str());
  Set("FriendsMagenta", ids[3].c_str());
}
