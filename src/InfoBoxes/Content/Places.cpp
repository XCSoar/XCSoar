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
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Task/ProtectedTaskManager.hpp"
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

static GlideResult
ComputeActiveWaypointGlide(const MoreData &basic,
                           const ComputerSettings &settings,
                           const DerivedInfo &calculated,
                           const Waypoint &waypoint) noexcept
{
  const GlideState glide_state(
    basic.location.DistanceBearing(waypoint.location),
    waypoint.elevation + settings.task.safety_height_arrival,
    basic.nav_altitude,
    calculated.GetWindOrZero());

  return MacCready::Solve(settings.task.glide,
                          settings.polar.glide_polar_task,
                          glide_state);
}

/* Shared by the Active and Previous Waypoint InfoBoxes so the "place"
   InfoBoxes display name / arrival height / distance consistently. */
static void
SetInfoBoxWaypointGlideData(InfoBoxData &data, const MoreData &basic,
                            const ComputerSettings &settings,
                            const DerivedInfo &calculated,
                            const Waypoint &waypoint) noexcept
{
  data.SetTitle(waypoint.name.c_str());

  const GlideResult result =
    ComputeActiveWaypointGlide(basic, settings, calculated, waypoint);
  data.SetValueFromArrival(result.SelectAltitudeDifference(settings.task.glide));
  data.SetCommentFromDistance(basic.location.DistanceS(waypoint.location));
}

void
InfoBoxContentActiveWaypoint::Update(InfoBoxData &data) noexcept
{
  const MoreData &basic = CommonInterface::Basic();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  data.SetTitle(_("Active WP"));

  const auto waypoint =
    (backend_components && backend_components->protected_task_manager)
    ? backend_components->protected_task_manager->GetActiveWaypoint()
    : nullptr;

  if (!waypoint ||
      !basic.location_available ||
      !basic.NavAltitudeAvailable() ||
      !settings.polar.glide_polar_task.IsValid()) {
    data.SetInvalid();
    data.SetCommentInvalid();
    return;
  }

  SetInfoBoxWaypointGlideData(data, basic, settings, calculated, *waypoint);
}

bool
InfoBoxContentActiveWaypoint::HandleClick() noexcept
{
  if (!data_components || !data_components->waypoints)
    return false;
  if (!backend_components || !backend_components->protected_task_manager)
    return false;

  auto &waypoints = *data_components->waypoints;
  auto &ptm = *backend_components->protected_task_manager;
  const GeoPoint location = CommonInterface::Basic().location;

  std::unique_ptr<OrderedTask> task_clone = ptm.TaskClone();

  bool task_mode_ordered = false;
  unsigned ordered_task_index = 0;
  {
    ProtectedTaskManager::Lease lease(ptm);
    task_mode_ordered = lease->GetMode() == TaskType::ORDERED;
    if (task_mode_ordered)
      ordered_task_index = lease->GetActiveTaskPointIndex();
  }

  const bool prepopulate_with_task = task_mode_ordered && task_clone &&
                                     task_clone->TaskSize() > 0;

  WaypointPtr selected;
  try {
    selected = ShowWaypointListDialog(waypoints, location,
                                      prepopulate_with_task ? task_clone.get()
                                                            : nullptr,
                                      ordered_task_index,
                                      std::nullopt,
                                      prepopulate_with_task);
  } catch (...) {
    return false;
  }
  if (!selected)
    return true;

  int target_index = -1;
  if (prepopulate_with_task) {
    for (unsigned i = 0; i < task_clone->TaskSize(); ++i) {
      if (task_clone->GetPoint(i).GetWaypointPtr() == selected) {
        target_index = (int)i;
        break;
      }
    }
  }

  if (target_index >= 0) {
    std::unique_ptr<OrderedTask> current_task = ptm.TaskClone();
    const unsigned current_active_index =
      (current_task && current_task->TaskSize() > 0)
        ? current_task->GetActiveIndex()
        : ordered_task_index;
    if ((unsigned)target_index != current_active_index)
      ptm.IncrementActiveTaskPoint(target_index - (int)current_active_index);
  } else {
    {
      ScopeSuspendAllThreads suspend;
      waypoints.EraseTempGoto();
    }
    ptm.DoGoto(std::move(selected));
  }

  return true;
}

void
InfoBoxContentPreviousWaypoint::Update(InfoBoxData &data) noexcept
{
  const MoreData &basic = CommonInterface::Basic();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();
  const DerivedInfo &calculated = CommonInterface::Calculated();

  data.SetTitle(_("Prev WP"));

  WaypointPtr waypoint = override_waypoint;
  if (waypoint == nullptr && backend_components &&
      backend_components->protected_task_manager) {
    /* Read the active task point under a lease instead of cloning the
       whole task on every tick: we only need a single waypoint pointer. */
    ProtectedTaskManager::Lease lease{*backend_components->protected_task_manager};
    if (lease->GetMode() == TaskType::ORDERED) {
      const OrderedTask &task = lease->GetOrderedTask();
      if (task.TaskSize() > 0) {
        const unsigned active_index = task.GetActiveIndex();
        const unsigned target_index = active_index > 0 ? active_index - 1 : 0;
        waypoint = task.GetPoint(target_index).GetWaypointPtr();
      }
    }
  }

  if (!waypoint ||
      !basic.location_available ||
      !basic.NavAltitudeAvailable() ||
      !settings.polar.glide_polar_task.IsValid()) {
    data.SetInvalid();
    data.SetCommentInvalid();
    return;
  }

  SetInfoBoxWaypointGlideData(data, basic, settings, calculated, *waypoint);
}

bool
InfoBoxContentPreviousWaypoint::HandleClick() noexcept
{
  if (!data_components || !data_components->waypoints)
    return false;
  if (!backend_components || !backend_components->protected_task_manager)
    return false;

  auto &waypoints = *data_components->waypoints;
  auto &ptm = *backend_components->protected_task_manager;
  const GeoPoint location = CommonInterface::Basic().location;

  std::unique_ptr<OrderedTask> task_clone = ptm.TaskClone();

  bool task_mode_ordered = false;
  {
    ProtectedTaskManager::Lease lease(ptm);
    task_mode_ordered = lease->GetMode() == TaskType::ORDERED;
  }

  const bool has_ordered_task = task_mode_ordered && task_clone &&
                                task_clone->TaskSize() > 0;
  const unsigned ordered_task_index =
    has_ordered_task ? task_clone->GetActiveIndex() : 0;

  /* "Resume auto tracking" only makes sense when a task is loaded *and*
     we currently have a manual override to clear. */
  const char *const action_label =
    (has_ordered_task && override_waypoint != nullptr)
      ? _("Resume auto tracking")
      : nullptr;
  bool action_selected = false;

  WaypointPtr selected;
  try {
    selected = ShowWaypointListDialog(waypoints, location,
                                      has_ordered_task ? task_clone.get()
                                                       : nullptr,
                                      ordered_task_index,
                                      std::nullopt,
                                      has_ordered_task,
                                      action_label,
                                      &action_selected);
  } catch (...) {
    return false;
  }

  if (action_selected)
    override_waypoint.reset();
  else if (selected)
    override_waypoint = std::move(selected);
  /* else: cancelled - leave override_waypoint unchanged. */

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
