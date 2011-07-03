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

#include "NOAAGlue.hpp"

#if defined(WIN32) && !defined(_WIN32_WCE) && !defined(HAVE_POSIX)

#include "NOAAStore.hpp"
#include "Profile/Profile.hpp"

#include <tchar.h>

namespace NOAAStore
{
  bool LoadFromString(const TCHAR *string);
};

bool
NOAAStore::LoadFromString(const TCHAR *string)
{
  TCHAR code[5];
  for (unsigned i = 0; *string != '\0'; string++) {
    // New code
    if (*string == ',' && i == 4) {
      code[4] = '\0';
      NOAAStore::AddStation(code);

      i = 0;
      continue;
    }

    // Skip additional characters
    // Skip characters outside of A-Z
    if (i >= 4 || *string <= 'A' || *string >= 'Z')
      continue;

    code[i] = *string;
    i++;
  }

  return true;
}

bool
NOAAStore::LoadFromProfile()
{
  TCHAR buffer[120];
  if (!Profile::Get(szProfileWeatherStations, buffer, 120))
    return false;

  return LoadFromString(buffer);
}

void
NOAAStore::SaveToProfile()
{
  TCHAR buffer[120];
  for (unsigned i = 0; i < NOAAStore::Count(); i++) {
    const TCHAR *code = NOAAStore::GetCodeT(i);
    _tcscpy(buffer + (i * 5), code);
    _tcscpy(buffer + (i * 5) + 4, _T(","));
  }

  Profile::Set(szProfileWeatherStations, buffer);
}

#else

bool
NOAAStore::LoadFromProfile()
{
  return false;
}

void
NOAAStore::SaveToProfile()
{
}

#endif
