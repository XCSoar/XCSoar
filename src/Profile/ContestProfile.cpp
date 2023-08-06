// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ContestProfile.hpp"
#include "Keys.hpp"
#include "Map.hpp"
#include "Engine/Contest/Settings.hpp"

void
Profile::Load(const ProfileMap &map, ContestSettings &settings)
{
  if (map.GetEnum(ProfileKeys::OLCRules, settings.contest)) {
    /* handle out-dated Sprint rule in profile */
    if (settings.contest == Contest::OLC_SPRINT)
      settings.contest = Contest::OLC_LEAGUE;
  }

  map.Get(ProfileKeys::PredictContest, settings.predict);
}
