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

#include "PlaneGlue.hpp"
#include "PlaneFileGlue.hpp"
#include "Plane.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Map.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarGlue.hpp"
#include "Computer/Settings.hpp"
#include "OS/Path.hpp"
#include "Util/Clamp.hpp"

void
PlaneGlue::FromProfile(Plane &plane, const ProfileMap &profile)
{
  {
    auto plane_path = profile.GetPath("PlanePath");
    if (!plane_path.IsNull() &&
        PlaneGlue::ReadFile(plane, plane_path))
      return;
  }

  /* the following is just here to load a pre-6.7 profile */

  plane.registration.SetUTF8(profile.Get(ProfileKeys::AircraftReg, ""));
  plane.competition_id.SetUTF8(profile.Get(ProfileKeys::CompetitionId, ""));
  plane.type.SetUTF8(profile.Get(ProfileKeys::AircraftType, ""));
  plane.polar_name.SetUTF8(profile.Get(ProfileKeys::PolarName, ""));

  PolarInfo polar = PolarGlue::LoadFromProfile();
  plane.polar_shape = polar.shape;
  plane.reference_mass = polar.reference_mass;
  plane.max_ballast = polar.max_ballast;
  plane.wing_area = polar.wing_area;

  if (polar.v_no > 0)
    plane.max_speed = polar.v_no;
  else if (!profile.Get(ProfileKeys::SafteySpeed, plane.max_speed))
    plane.max_speed = 0;

  if (!profile.Get(ProfileKeys::DryMass, plane.dry_mass))
    plane.dry_mass = plane.reference_mass;

  if (!profile.Get(ProfileKeys::BallastSecsToEmpty, plane.dump_time))
    plane.dump_time = 120;

  if (!profile.Get(ProfileKeys::Handicap, plane.handicap))
    plane.handicap = 100;
}

void
PlaneGlue::Synchronize(const Plane &plane, ComputerSettings &settings,
                       GlidePolar &gp)
{
  settings.contest.handicap = plane.handicap;

  PolarCoefficients pc = plane.polar_shape.CalculateCoefficients();
  if (!pc.IsValid())
    return;

  gp.SetCoefficients(pc, false);

  // Glider empty weight
  gp.SetReferenceMass(plane.reference_mass, false);
  gp.SetDryMass(plane.dry_mass, false);

  // Ballast weight
  gp.SetBallastRatio(plane.max_ballast / plane.reference_mass);

  gp.SetWingArea(plane.wing_area);

  // assure the polar is not invalid (because of VMin > VMax)
  gp.SetVMax(75, false);

  gp.Update();

  // set VMax from settings but assure the range [VMin, VMax] is reasonable
  if (plane.max_speed > 0)
    gp.SetVMax(Clamp(plane.max_speed, gp.GetVMin() + 10, 75.));

  settings.plane.competition_id = plane.competition_id;
  settings.plane.registration = plane.registration;
  settings.plane.type = plane.type;
}
