// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskProfile.hpp"
#include "RouteProfile.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "Task/TaskBehaviour.hpp"

namespace Profile {
  static void Load(const ProfileMap &map, GlideSettings &settings);
  static void Load(const ProfileMap &map, TaskStartMargins &settings);
  static void Load(const ProfileMap &map, SectorDefaults &settings);
  static void Load(const ProfileMap &map, StartConstraints &constraints);
  static void Load(const ProfileMap &map, FinishConstraints &constraints);
  static void Load(const ProfileMap &map, OrderedTaskSettings &settings);
};

void
Profile::Load(const ProfileMap &map, GlideSettings &settings)
{
  map.Get(ProfileKeys::PredictWindDrift, settings.predict_wind_drift);
}

void
Profile::Load(const ProfileMap &map, TaskStartMargins &settings)
{
  map.Get(ProfileKeys::StartMaxHeightMargin, settings.max_height_margin);
  map.Get(ProfileKeys::StartMaxSpeedMargin, settings.max_speed_margin);
}

void
Profile::Load(const ProfileMap &map, SectorDefaults &settings)
{
  map.GetEnum(ProfileKeys::StartType, settings.start_type);
  map.Get(ProfileKeys::StartRadius, settings.start_radius);
  map.GetEnum(ProfileKeys::TurnpointType, settings.turnpoint_type);
  map.Get(ProfileKeys::TurnpointRadius, settings.turnpoint_radius);
  map.GetEnum(ProfileKeys::FinishType, settings.finish_type);
  map.Get(ProfileKeys::FinishRadius, settings.finish_radius);
}

void
Profile::Load(const ProfileMap &map, StartConstraints &constraints)
{
  map.GetEnum(ProfileKeys::StartHeightRef, constraints.max_height_ref);
  map.Get(ProfileKeys::StartMaxHeight, constraints.max_height);
  map.Get(ProfileKeys::StartMaxSpeed, constraints.max_speed);
  map.Get(ProfileKeys::PEVStartWaitTime, constraints.pev_start_wait_time);
  map.Get(ProfileKeys::PEVStartWindow, constraints.pev_start_window);
}

void
Profile::Load(const ProfileMap &map, FinishConstraints &constraints)
{
  map.GetEnum(ProfileKeys::FinishHeightRef, constraints.min_height_ref);
  map.Get(ProfileKeys::FinishMinHeight, constraints.min_height);
}

void
Profile::Load(const ProfileMap &map, OrderedTaskSettings &settings)
{
  Load(map, settings.start_constraints);
  Load(map, settings.finish_constraints);
  map.Get(ProfileKeys::AATMinTime, settings.aat_min_time);
}

void
Profile::Load(const ProfileMap &map, TaskBehaviour &settings)
{
  Load(map, settings.glide);

  map.Get(ProfileKeys::AATTimeMargin, settings.optimise_targets_margin);
  map.Get(ProfileKeys::AutoMc, settings.auto_mc);
  map.GetEnum(ProfileKeys::AutoMcMode, settings.auto_mc_mode);

  unsigned Temp;
  if (map.Get(ProfileKeys::RiskGamma, Temp))
    settings.risk_gamma = Temp / 10.;

  if (map.Get(ProfileKeys::SafetyMacCready, Temp))
    settings.safety_mc = Temp / 10.;

  map.Get(ProfileKeys::SafetyAltitudeArrival, settings.safety_height_arrival);
  map.GetEnum(ProfileKeys::TaskType, settings.task_type_default);
  Load(map, settings.start_margins);

  Load(map, settings.sector_defaults);
  Load(map, settings.ordered_defaults);

  map.GetEnum(ProfileKeys::AbortTaskMode, settings.abort_task_mode);

  Load(map, settings.route_planner);
}
