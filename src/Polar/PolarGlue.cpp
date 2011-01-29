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

#include "Polar/PolarGlue.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Profile/Profile.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/ConfiguredFile.hpp"

namespace PolarGlue
{
  bool LoadFromOldProfile(SimplePolar &polar);
}

bool
PolarGlue::LoadFromFile(SimplePolar &polar, TLineReader &reader)
{
  const TCHAR *line;
  while ((line = reader.read()) != NULL)
    if (polar.ReadString(line))
      return true;

  return false;
}

bool
PolarGlue::LoadFromFile(SimplePolar &polar, const TCHAR* path)
{
  FileLineReader *reader = new FileLineReader(path);
  if (reader == NULL)
    return false;

  if (reader->error()) {
    delete reader;
    return false;
  }

  LoadFromFile(polar, *reader);
  delete reader;
  return true;
}

static bool
ReadPolarFileFromProfile(SimplePolar &polar)
{
  TLineReader *reader = OpenConfiguredTextFile(szProfilePolarFile);
  if (reader == NULL)
    return false;

  bool success = PolarGlue::LoadFromFile(polar, *reader);
  delete reader;

  return success;
}

bool
PolarGlue::LoadFromOldProfile(SimplePolar &polar)
{
  unsigned Temp;
  if (Profile::Get(szProfilePolarID, Temp)) {
    if (Temp == 6) {
      if (ReadPolarFileFromProfile(polar))
        return true;
    } else {
      switch (Temp) {
      case 0: // Ka 6
        Temp = 45;
        break;

      case 1: // ASW 19
        Temp = 16;
        break;

      case 2: // LS 8
        Temp = 56;
        break;

      case 3: // ASW 27
        Temp = 19;
        break;

      case 4: // LS 6c
        Temp = 55;
        break;

      case 5: // ASW 22
        Temp = 118;
        break;
      }

      if (PolarStore::Read(Temp, polar))
        return true;
    }
  }
  return false;
}

void
PolarGlue::LoadFromProfile(SimplePolar &polar)
{
  TCHAR polar_string[255] = _T("\0");
  if (Profile::Get(szProfilePolar, polar_string, 255) &&
      polar_string[0] != 0 &&
      polar.ReadString(polar_string))
    return;

  if (LoadFromOldProfile(polar))
    return;

  polar.ReadString(_T("325, 185, 70, -0.51, 115, -0.85, 173, -2.00, 10.5"));
}

void
PolarGlue::SaveToProfile(SimplePolar &polar)
{
  TCHAR polar_string[255];
  polar.GetString(polar_string, 255);
  Profile::Set(szProfilePolar, polar_string);
}
