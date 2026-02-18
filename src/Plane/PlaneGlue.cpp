// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlaneGlue.hpp"
#include "PlaneFileGlue.hpp"
#include "Plane.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Map.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarGlue.hpp"
#include "Computer/Settings.hpp"
#include "system/Path.hpp"

#include <algorithm> // for std::clamp()

void
PlaneGlue::FromProfile(Plane &plane, const ProfileMap &profile) noexcept
{
  {
    auto plane_path = profile.GetPath("PlanePath");
    if (plane_path != nullptr &&
        PlaneGlue::ReadFile(plane, plane_path))
      return;
  }

  /* No plane profile file loaded - default plane is active */
  plane.plane_profile_active = false;

  /* the following is just here to load a pre-6.7 profile */

  plane.registration.SetUTF8(profile.Get(ProfileKeys::AircraftReg, ""));
  plane.competition_id.SetUTF8(profile.Get(ProfileKeys::CompetitionId, ""));
  plane.type.SetUTF8(profile.Get(ProfileKeys::AircraftType, ""));
  plane.polar_name.SetUTF8(profile.Get(ProfileKeys::PolarName, ""));

  /* Don't load default polar when no plane file is found - wait for device POLAR data instead */
  /* Only load polar from profile if explicitly configured (for backward compatibility) */
  PolarInfo polar;
  bool polar_loaded = false;
  if (PolarGlue::LoadFromProfile(polar) && polar.IsValid()) {
    /* User has explicitly configured a polar in profile, use it */
    plane.polar_shape = polar.shape;
    plane.max_ballast = polar.max_ballast;
    plane.wing_area = polar.wing_area;
    polar_loaded = true;
  } else {
    /* No plane file and no valid polar in profile - leave plane structure empty
     * so device POLAR data can be applied instead of default polar */
    plane.polar_shape.reference_mass = 0;
    plane.max_ballast = 0;
    plane.wing_area = 0;
  }

  if (polar_loaded && polar.v_no > 0)
    plane.max_speed = polar.v_no;
  else if (!profile.Get(ProfileKeys::SafteySpeed, plane.max_speed))
    plane.max_speed = 0;

  double crew_mass; // for compatibility reasons, to be removed later
  if (profile.Get(ProfileKeys::CrewWeightTemplate, crew_mass))
    plane.dry_mass_obsolete = plane.empty_mass + crew_mass;

  if (!profile.Get(ProfileKeys::BallastSecsToEmpty, plane.dump_time))
    plane.dump_time = 120;

  if (!profile.Get(ProfileKeys::Handicap, plane.handicap))
    plane.handicap = 100;

  if (!profile.Get(ProfileKeys::WeGlideAircraftType, plane.weglide_glider_type))
    plane.weglide_glider_type = 0;
}

void
PlaneGlue::Synchronize(const Plane &plane, ComputerSettings &settings,
                       GlidePolar &gp) noexcept
{
  settings.contest.handicap = plane.handicap;
  settings.plane.competition_id = plane.competition_id;
  settings.plane.registration = plane.registration;
  settings.plane.type = plane.type;
  settings.plane.weglide_glider_type = plane.weglide_glider_type;

  /* Don't apply polar if no plane profile is loaded (reference_mass == 0)
   * - wait for device POLAR data instead */
  if (plane.polar_shape.reference_mass <= 0)
    return;

  PolarCoefficients pc = plane.polar_shape.CalculateCoefficients();
  if (!pc.IsValid())
    return;

  gp.SetCoefficients(pc, false);

  gp.SetReferenceMass(plane.polar_shape.reference_mass, false);
  gp.SetEmptyMass(plane.empty_mass, false);

  // Ballast weight - set max_ballast directly (configured value)
  gp.SetMaxBallast(plane.max_ballast, false);

  gp.SetWingArea(plane.wing_area);

  gp.SetVMax(DEFAULT_MAX_SPEED, false);

  gp.Update();

  if (plane.max_speed > 0)
    gp.SetVMax(std::clamp(plane.max_speed,
                          gp.GetVMin() + 10, DEFAULT_MAX_SPEED));
}
