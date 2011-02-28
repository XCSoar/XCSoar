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
#include "IO/TextWriter.hpp"
#include "IO/ConfiguredFile.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"

namespace PolarGlue
{
  bool LoadFromOldProfile(SimplePolar &polar);
}

void
PolarGlue::LoadDefault(SimplePolar &polar)
{
  // Load LS8 polar
  PolarStore::Read(56, polar);
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

bool
PolarGlue::SaveToFile(const SimplePolar &polar, TextWriter &writer)
{
  TCHAR buffer[256];
  polar.GetString(buffer, 256);
  return writer.writeln(buffer);
}

bool
PolarGlue::SaveToFile(const SimplePolar &polar, const TCHAR* path)
{
  TextWriter writer(path);
  if (writer.error())
    return false;

  return SaveToFile(polar, writer);
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
      if (Temp < 6)
        Temp = (Temp == 0) ? 52 :
               (Temp == 1) ? 23 :
               (Temp == 2) ? 63 :
               (Temp == 3) ? 26 :
               (Temp == 4) ? 62 :
               125;

      PolarStore::Read(Temp - 7, polar);
      return true;
    }
  }
  return false;
}

bool
PolarGlue::LoadFromProfile(SimplePolar &polar)
{
  TCHAR polar_string[255] = _T("\0");
  if (Profile::Get(szProfilePolar, polar_string, 255) &&
      polar_string[0] != 0 &&
      polar.ReadString(polar_string))
    return true;

  return LoadFromOldProfile(polar);
}

void
PolarGlue::SaveToProfile(const SimplePolar &polar)
{
  TCHAR polar_string[255];
  polar.GetString(polar_string, 255);
  Profile::Set(szProfilePolar, polar_string);
}

void
PolarGlue::LoadFromProfile(GlidePolar &gp)
{
  SimplePolar polar;
  if (!LoadFromProfile(polar) || !polar.CopyIntoGlidePolar(gp)) {
    MessageBoxX(_("Polar has invalid coefficients.\nUsing LS8 polar instead!"),
                _("Warning"), MB_OK);
    LoadDefault(polar);
    polar.CopyIntoGlidePolar(gp);
  }
}
