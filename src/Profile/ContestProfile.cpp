// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ContestProfile.hpp"
#include "Keys.hpp"
#include "Map.hpp"
#include "Engine/Contest/Settings.hpp"

void
Profile::Load(const ProfileMap &map, ContestSettings &settings)
{
  unsigned contest_enum_layout = 0;
  const bool have_contest_layout =
      map.Get(ProfileKeys::ContestEnumLayout, contest_enum_layout);

  if (map.GetEnum(ProfileKeys::OLCRules, settings.contest)) {
    /* handle out-dated Sprint rule in profile */
    if (settings.contest == Contest::OLC_SPRINT)
      settings.contest = Contest::OLC_LEAGUE;

    /**
     * v7.44 removed NET_COUPE from the enum: stored NONE was 14 while CHARRON
     * was 13, matching the layout again after NET_COUPE was reinserted before
     * NONE.  Profiles without #ContestEnumLayout still use that encoding for
     * the last slot only.
     */
    if (!have_contest_layout || contest_enum_layout < 2) {
      const unsigned v = static_cast<unsigned>(settings.contest);
      if (v == 14U)
        settings.contest = Contest::NONE;
    }
  }

  map.Get(ProfileKeys::PredictContest, settings.predict);
}
