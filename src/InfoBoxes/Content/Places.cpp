// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Places.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/ATCReference.hpp"
#include "InfoBoxes/Panel/ATCSetup.hpp"
#include "Interface.hpp"
#include "NMEA/MoreData.hpp"
#include "Language/Language.hpp"
#include "Formatter/Units.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Protection.hpp"
#include "Profile/Current.hpp"
#include "Profile/Profile.hpp"

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

static GlideResult
ComputeHomeGlide(const MoreData &basic,
                 const ComputerSettings &settings,
                 const DerivedInfo &calculated) noexcept
{
  const GlideState glide_state(
    basic.location.DistanceBearing(settings.poi.home_location),
    settings.poi.home_elevation + settings.task.safety_height_arrival,
    basic.nav_altitude,
    calculated.GetWindOrZero());

  return MacCready::Solve(settings.task.glide,
                          settings.polar.glide_polar_task,
                          glide_state);
}

void
UpdateInfoBoxHomeAltitudeDiff(InfoBoxData &data) noexcept
{
  const MoreData &basic = CommonInterface::Basic();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (!basic.location_available ||
      !basic.NavAltitudeAvailable() ||
      !settings.polar.glide_polar_task.IsValid() ||
      !common_stats.vector_home.IsValid() ||
      !settings.poi.home_location_available ||
      !settings.poi.home_elevation_available) {
    data.SetInvalid();
    data.SetCommentInvalid();
    return;
  }

  const GlideResult result = ComputeHomeGlide(basic, settings, calculated);
  data.SetValueFromArrival(result.SelectAltitudeDifference(settings.task.glide));
  data.SetCommentFromDistance(common_stats.vector_home.distance);
}

void
InfoBoxContentHome::Update(InfoBoxData &data) noexcept
{
  const MoreData &basic = CommonInterface::Basic();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const CommonStats &common_stats = CommonInterface::Calculated().common_stats;
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (!basic.location_available ||
      !basic.NavAltitudeAvailable() ||
      !settings.polar.glide_polar_task.IsValid() ||
      !common_stats.vector_home.IsValid() ||
      !settings.poi.home_location_available ||
      !settings.poi.home_elevation_available) {
    data.SetInvalid();
    data.SetCommentInvalid();
    return;
  }

  data.SetTitle(_("Home"));
  if (data_components && data_components->waypoints) {
    auto wp = data_components->waypoints->LookupId(settings.poi.home_waypoint);
    if (wp)
      data.SetTitle(wp->name.c_str());
  }

  const GlideResult result = ComputeHomeGlide(basic, settings, calculated);
  data.SetValueFromArrival(result.SelectAltitudeDifference(settings.task.glide));
  data.SetCommentFromDistance(common_stats.vector_home.distance);
}

bool
InfoBoxContentHome::HandleClick() noexcept
{
  if (!data_components || !data_components->waypoints)
    return false;

  if (!backend_components)
    return false;

  const GeoPoint location = CommonInterface::Basic().location;
  WaypointPtr waypoint;
  try {
    waypoint = ShowWaypointListDialog(*data_components->waypoints, location);
  } catch (...) {
    return false;
  }
  if (!waypoint)
    return true;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();

  {
    ScopeSuspendAllThreads suspend;
    settings_computer.poi.SetHome(*waypoint);
    WaypointGlue::SetHome(*data_components->waypoints,
                          data_components->terrain.get(),
                          settings_computer.poi, settings_computer.team_code,
                          backend_components->device_blackboard.get(), false);
    WaypointGlue::SaveHome(Profile::map,
                           settings_computer.poi, settings_computer.team_code);
  }

  Profile::Save();
  return true;
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
