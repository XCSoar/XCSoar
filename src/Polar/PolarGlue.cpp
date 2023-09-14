// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
PolarGlue::GetDefault() noexcept
{
  // Return LS8 polar
  return PolarStore::GetDefault().ToPolarInfo();
}

bool
PolarGlue::LoadFromProfile(PolarInfo &polar) noexcept
{
  const char *polar_string = Profile::Get(ProfileKeys::Polar);
  if (polar_string != nullptr && !StringIsEmpty(polar_string) &&
      ParsePolar(polar, polar_string)) {
    return true;
  }

  return false;
}

PolarInfo
PolarGlue::LoadFromProfile() noexcept
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
