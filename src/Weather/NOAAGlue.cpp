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

#include "NOAAGlue.hpp"
#include "NOAAStore.hpp"
#include "Profile/Profile.hpp"

#include <algorithm>

NOAAStore *noaa_store;

bool
NOAAStore::LoadFromString(const char *string)
{
  const char *s = string;
  while (s != NULL && *s) {
    const char *next = strchr(s, _T(','));
    if ((next != NULL && next - s == 4) || (next == NULL && strlen(s) == 4)) {
      char code[5];
      std::copy_n(s, 4, code);
      code[4] = '\0';
      if (IsValidCode(code))
        AddStation(code);
    }
    s = (next == NULL) ? NULL : next + 1;
  }
  return true;
}

bool
NOAAStore::LoadFromProfile()
{
  const char *stations = Profile::Get(ProfileKeys::WeatherStations);
  if (stations == NULL)
    return false;

  return LoadFromString(stations);
}

void
NOAAStore::SaveToProfile()
{
  char buffer[120], *p = buffer;
  for (auto i = begin(), e = end(); i != e; ++i) {
    const char *code = i->code;
    p = std::copy_n(code, strlen(code), p);
    *p++ = _T(',');
  }

  *p = _T('\0');

  Profile::Set(ProfileKeys::WeatherStations, buffer);
}
