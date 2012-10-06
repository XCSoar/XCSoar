/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Profile/TaskProfile.hpp"
#include "Profile/RouteProfile.hpp"
#include "Profile/Profile.hpp"
#include "Task/TaskBehaviour.hpp"

namespace Profile {
  static void Load(GlideSettings &settings);
  static void Load(TaskStartMargins &settings);
  static void Load(SectorDefaults &settings);
  static void Load(StartConstraints &constraints);
  static void Load(FinishConstraints &constraints);
  static void Load(OrderedTaskBehaviour &settings);
};

void
Profile::Load(GlideSettings &settings)
{
  Get(ProfileKeys::PredictWindDrift, settings.predict_wind_drift);
}

void
Profile::Load(TaskStartMargins &settings)
{
  Get(ProfileKeys::StartMaxHeightMargin, settings.max_height_margin);
  Get(ProfileKeys::StartMaxSpeedMargin, settings.max_speed_margin);
}

void
Profile::Load(SectorDefaults &settings)
{
  GetEnum(ProfileKeys::StartType, settings.start_type);
  Get(ProfileKeys::StartRadius, settings.start_radius);
  GetEnum(ProfileKeys::TurnpointType, settings.turnpoint_type);
  Get(ProfileKeys::TurnpointRadius, settings.turnpoint_radius);
  GetEnum(ProfileKeys::FinishType, settings.finish_type);
  Get(ProfileKeys::FinishRadius, settings.finish_radius);
}

void
Profile::Load(StartConstraints &constraints)
{
  GetEnum(ProfileKeys::StartHeightRef, constraints.max_height_ref);
  Get(ProfileKeys::StartMaxHeight, constraints.max_height);
  Get(ProfileKeys::StartMaxSpeed, constraints.max_speed);
}

void
Profile::Load(FinishConstraints &constraints)
{
  GetEnum(ProfileKeys::FinishHeightRef, constraints.min_height_ref);
  Get(ProfileKeys::FinishMinHeight, constraints.min_height);
}

void
Profile::Load(OrderedTaskBehaviour &settings)
{
  Load(settings.start_constraints);
  Load(settings.finish_constraints);
  Get(ProfileKeys::AATMinTime, settings.aat_min_time);
}

void
Profile::Load(TaskBehaviour &settings)
{
  Load(settings.glide);

  Get(ProfileKeys::AATTimeMargin, settings.optimise_targets_margin);
  Get(ProfileKeys::AutoMc, settings.auto_mc);
  GetEnum(ProfileKeys::AutoMcMode, settings.auto_mc_mode);

  unsigned Temp;
  if (Get(ProfileKeys::RiskGamma, Temp))
    settings.risk_gamma = fixed(Temp) / 10;

  if (Get(ProfileKeys::SafetyMacCready, Temp))
    settings.safety_mc = fixed(Temp) / 10;

  Get(ProfileKeys::SafetyAltitudeArrival, settings.safety_height_arrival);
  GetEnum(ProfileKeys::TaskType, settings.task_type_default);
  Load(settings.start_margins);

  Load(settings.sector_defaults);
  Load(settings.ordered_defaults);

  GetEnum(ProfileKeys::AbortTaskMode, settings.abort_task_mode);

  Load(settings.route_planner);
}
