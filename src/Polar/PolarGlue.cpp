/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "io/ConfiguredFile.hpp"
#include "io/LineReader.hpp"
#include "Dialogs/Message.hpp"
#include "Language/Language.hpp"
#include "util/StringCompare.hxx"

#include <memory>

PolarInfo
PolarGlue::GetDefault()
{
  // Return LS8 polar
  return PolarStore::GetDefault().ToPolarInfo();
}

bool
PolarGlue::LoadFromProfile(PolarInfo &polar)
{
  const char *polar_string = Profile::Get(ProfileKeys::Polar);
  if (polar_string != nullptr && !StringIsEmpty(polar_string) &&
      ParsePolar(polar, polar_string)) {
    return true;
  }

  return false;
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
