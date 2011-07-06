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

#include "PlaneGlue.hpp"
#include "Plane.hpp"
#include "Profile/Profile.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarGlue.hpp"
#include "SettingsComputer.hpp"

void
PlaneGlue::FromProfile(Plane &plane)
{
  if (!Profile::Get(szProfileAircraftReg, plane.registration))
    plane.registration.clear();

  if (!Profile::Get(szProfileCompetitionId, plane.competition_id))
    plane.competition_id.clear();

  if (!Profile::Get(szProfileAircraftType, plane.type))
    plane.type.clear();

  if (!Profile::Get(szProfilePolarName, plane.polar_name))
    plane.polar_name.clear();

  if (!Profile::Get(szProfileBallastSecsToEmpty, plane.dump_time))
    plane.dump_time = 120;

  if (!Profile::Get(szProfileHandicap, plane.handicap))
    plane.handicap = 100;
}

void
PlaneGlue::ToProfile(const Plane &plane)
{
  Profile::Set(szProfileAircraftReg, plane.registration);
  Profile::Set(szProfileCompetitionId, plane.competition_id);
  Profile::Set(szProfileAircraftType, plane.type);

  Profile::Set(szProfilePolarName, plane.polar_name);

  Profile::Set(szProfileBallastSecsToEmpty, plane.dump_time);
  Profile::Set(szProfileHandicap, plane.handicap);
}

void
PlaneGlue::Synchronize(const Plane &plane, SETTINGS_COMPUTER &settings)
{
  settings.contest_handicap = plane.handicap;
}
