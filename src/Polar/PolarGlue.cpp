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

#include "Polar/PolarGlue.hpp"
#include "Polar/PolarFileGlue.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Parser.hpp"
#include "Profile/Profile.hpp"
#include "IO/ConfiguredFile.hpp"
#include "IO/LineReader.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "Util/StringCompare.hxx"

#include <memory>

namespace PolarGlue
{
  bool LoadFromOldProfile(PolarInfo &polar);
}

PolarInfo
PolarGlue::GetDefault()
{
  // Return LS8 polar
  return PolarStore::GetItem(56).ToPolarInfo();
}

static bool
ReadPolarFileFromProfile(PolarInfo &polar)
{
  auto reader = OpenConfiguredTextFileA(ProfileKeys::PolarFile);
  return reader && PolarGlue::LoadFromFile(polar, *reader);
}

bool
PolarGlue::LoadFromOldProfile(PolarInfo &polar)
{
  unsigned polar_id;
  if (!Profile::Get(ProfileKeys::PolarID, polar_id))
    return false;

  if (polar_id == 6)
    return ReadPolarFileFromProfile(polar);

  if (polar_id == 0)
    polar_id = 45;
  else if (polar_id == 1)
    polar_id = 16;
  else if (polar_id == 2)
    polar_id = 56;
  else if (polar_id == 3)
    polar_id = 19;
  else if (polar_id == 4)
    polar_id = 55;
  else if (polar_id == 5)
    polar_id = 118;
  else {
    polar_id -= 7;

    if (polar_id >= PolarStore::Count())
      return false;
  }

  polar = PolarStore::GetItem(polar_id).ToPolarInfo();
  return true;
}

bool
PolarGlue::LoadFromProfile(PolarInfo &polar)
{
  const char *polar_string = Profile::Get(ProfileKeys::Polar);
  if (polar_string != nullptr && !StringIsEmpty(polar_string) &&
      ParsePolar(polar, polar_string)) {
    return true;
  }

  return LoadFromOldProfile(polar);
}

PolarInfo
PolarGlue::LoadFromProfile()
{
  PolarInfo polar;
  if (!LoadFromProfile(polar) || !polar.IsValid()) {
    if (Profile::Exists(ProfileKeys::Polar) || Profile::Exists(ProfileKeys::PolarID))
      ShowMessageBox(_("Polar has invalid coefficients.\nUsing LS8 polar instead!"),
                  _("Warning"), MB_OK);
    polar = GetDefault();
  }

  return polar;
}
