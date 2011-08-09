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

#include "Profile/TaskProfile.hpp"
#include "Profile/RouteProfile.hpp"
#include "Profile/Profile.hpp"
#include "Task/TaskBehaviour.hpp"

namespace Profile {
  static void Load(TaskStartMargins &settings);
  static void Load(SectorDefaults &settings);
  static void Load(OrderedTaskBehaviour &settings);
};

void
Profile::Load(TaskStartMargins &settings)
{
  Get(szProfileStartMaxHeightMargin, settings.start_max_height_margin);
  Get(szProfileStartMaxSpeedMargin, settings.start_max_speed_margin);
}

void
Profile::Load(SectorDefaults &settings)
{
  GetEnum(szProfileStartType, settings.start_type);
  Get(szProfileStartRadius, settings.start_radius);
  GetEnum(szProfileTurnpointType, settings.turnpoint_type);
  Get(szProfileTurnpointRadius, settings.turnpoint_radius);
  GetEnum(szProfileFinishType, settings.finish_type);
  Get(szProfileFinishRadius, settings.finish_radius);
}

void
Profile::Load(OrderedTaskBehaviour &settings)
{
  GetEnum(szProfileFinishHeightRef, settings.finish_min_height_ref);
  Get(szProfileFinishMinHeight, settings.finish_min_height);
  GetEnum(szProfileStartHeightRef, settings.start_max_height_ref);
  Get(szProfileStartMaxHeight, settings.start_max_height);
  Get(szProfileStartMaxSpeed, settings.start_max_speed);
  Get(szProfileAATMinTime, settings.aat_min_time);
}

void
Profile::Load(TaskBehaviour &settings)
{
  Load((TaskStartMargins &)settings);

  Get(szProfileAATTimeMargin, settings.optimise_targets_margin);
  Get(szProfileAutoMc, settings.auto_mc);
  GetEnum(szProfileAutoMcMode, settings.auto_mc_mode);

  unsigned Temp;
  if (Get(szProfileRiskGamma, Temp))
    settings.risk_gamma = fixed(Temp) / 10;

  if (GetEnum(szProfileOLCRules, settings.contest)) {
    /* handle out-dated Sprint rule in profile */
    if (settings.contest == OLC_Sprint)
      settings.contest = OLC_League;
  }

  if (Get(szProfileSafetyMacCready, Temp))
    settings.safety_mc = fixed(Temp) / 10;

  Get(szProfileSafetyAltitudeArrival, settings.safety_height_arrival);
  GetEnum(szProfileTaskType, settings.task_type_default);

  Load(settings.sector_defaults);
  Load(settings.ordered_defaults);

  GetEnum(szProfileAbortTaskMode, settings.abort_task_mode);

  Load(settings.route_planner);
}
