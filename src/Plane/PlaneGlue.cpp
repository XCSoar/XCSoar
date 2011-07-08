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

  PolarInfo polar = PolarGlue::LoadFromProfile();
  plane.v1 = polar.v1;
  plane.v2 = polar.v2;
  plane.v3 = polar.v3;
  plane.w1 = polar.w1;
  plane.w2 = polar.w2;
  plane.w3 = polar.w3;
  plane.reference_mass = polar.reference_mass;
  plane.dry_mass = positive(polar.dry_mass) ? polar.dry_mass :
                                              polar.reference_mass;
  plane.max_ballast = polar.max_ballast;
  plane.max_speed = polar.v_no;
  plane.wing_area = polar.wing_area;

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

  PolarInfo polar;
  polar.v1 = plane.v1;
  polar.v2 = plane.v2;
  polar.v3 = plane.v3;
  polar.w1 = plane.w1;
  polar.w2 = plane.w2;
  polar.w3 = plane.w3;
  polar.reference_mass = plane.reference_mass;
  polar.max_ballast = plane.max_ballast;
  polar.v_no = plane.max_speed;
  polar.wing_area = plane.wing_area;

  TCHAR polar_string[255];
  polar.GetString(polar_string, 255, true);
  Profile::Set(szProfilePolar, polar_string);
  Profile::Set(szProfileDryMass, plane.dry_mass);

  Profile::Set(szProfileBallastSecsToEmpty, plane.dump_time);
  Profile::Set(szProfileHandicap, plane.handicap);
}

void
PlaneGlue::Synchronize(const Plane &plane, SETTINGS_COMPUTER &settings,
                       GlidePolar &gp)
{
  settings.contest_handicap = plane.handicap;

  PolarCoefficients pc = PolarCoefficients::FromVW(plane.v1, plane.v2, plane.v3,
                                                   plane.w1, plane.w2, plane.w3);
  if (!pc.IsValid())
    return;

  gp.SetCoefficients(pc, false);

  // Glider empty weight
  gp.SetReferenceMass(plane.reference_mass, false);
  gp.SetDryMass(plane.dry_mass, false);

  // Ballast weight
  gp.SetBallastRatio(plane.max_ballast / plane.reference_mass);

  gp.SetWingArea(plane.wing_area);

  gp.Update();
}
