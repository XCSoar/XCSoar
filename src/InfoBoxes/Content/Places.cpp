// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Places.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/ATCReference.hpp"
#include "InfoBoxes/Panel/ATCSetup.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Formatter/Units.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"

void
UpdateInfoBoxHomeDistance(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;

  if (!common_stats.vector_home.IsValid()) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValueFromDistance(common_stats.vector_home.distance);

  if (basic.track_available) {
    Angle bd = common_stats.vector_home.bearing - basic.track;
    data.SetCommentFromBearingDifference(bd);
  } else
    data.SetCommentInvalid();
}

void
UpdateInfoBoxHomeAltitudeDiff(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const MoreData &more_data = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (!basic.location_available ||
      !more_data.NavAltitudeAvailable() ||
      !settings.polar.glide_polar_task.IsValid() ||
      !common_stats.vector_home.IsValid() ||
      !settings.poi.home_location_available ||
      !settings.poi.home_elevation_available) {
    data.SetInvalid();
    data.SetCommentInvalid();
    return;
  }

  const GlideState glide_state(
    basic.location.DistanceBearing(settings.poi.home_location),
    settings.poi.home_elevation + settings.task.safety_height_arrival,
    more_data.nav_altitude,
    calculated.GetWindOrZero());

  const GlideResult &result =
    MacCready::Solve(settings.task.glide,
                     settings.polar.glide_polar_task,
                     glide_state);

  // Display altitude difference and distance.
  data.SetValueFromArrival(result.pure_glide_altitude_difference);
  data.SetCommentFromDistance(common_stats.vector_home.distance);
}

void
UpdateInfoBoxTakeoffDistance(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const FlyingState &flight = CommonInterface::Calculated().flight;

  if (!basic.location_available || !flight.flying ||
      !flight.takeoff_location.IsValid()) {
    data.SetInvalid();
    return;
  }

  const GeoVector vector(basic.location, flight.takeoff_location);
  data.SetValueFromDistance(vector.distance);

  if (basic.track_available)
    data.SetCommentFromBearingDifference(vector.bearing - basic.track);
  else
    data.SetCommentInvalid();
}

#ifdef __clang__
/* gcc gives "redeclaration differs in 'constexpr'" */
constexpr
#endif
const InfoBoxPanel atc_infobox_panels[] = {
  { N_("Reference"), LoadATCReferencePanel },
  { N_("Setup"), LoadATCSetupPanel },
  { nullptr, nullptr }
};

void
UpdateInfoBoxATCRadial(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const GeoPoint &reference =
    CommonInterface::GetComputerSettings().poi.atc_reference;
  const Angle &declination =
    CommonInterface::GetComputerSettings().poi.magnetic_declination;

  if (!basic.location_available || !reference.IsValid()) {
    data.SetInvalid();
    return;
  }

  const GeoVector vector(reference, basic.location);

  const Angle mag_bearing = vector.bearing - declination;
  data.SetValue(mag_bearing);

  FormatDistance(data.comment.buffer(), vector.distance,
                 Unit::NAUTICAL_MILES, true, 1);
}
