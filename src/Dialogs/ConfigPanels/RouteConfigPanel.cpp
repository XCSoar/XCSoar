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

#include "RouteConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

static WndForm* wf = NULL;

static void
ShowRouteControls(bool show)
{
  ShowFormControl(*wf, _T("prpRoutePlannerAllowClimb"), show);
  ShowFormControl(*wf, _T("prpRoutePlannerUseCeiling"), show);
}

static void
ShowReachControls(bool show)
{
  ShowFormControl(*wf, _T("prpFinalGlideTerrain"), show);
  ShowFormControl(*wf, _T("prpReachPolarMode"), show);
}

void
RouteConfigPanel::OnRouteMode(DataField *Sender,
                              DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  RoutePlannerConfig::Mode mode = (RoutePlannerConfig::Mode)df.GetAsInteger();
  ShowRouteControls(mode != RoutePlannerConfig::rpNone);
}

void
RouteConfigPanel::OnReachMode(DataField *Sender,
                              DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  RoutePlannerConfig::ReachMode mode =
    (RoutePlannerConfig::ReachMode)df.GetAsInteger();
  ShowReachControls(mode != RoutePlannerConfig::rmOff);
}

void
RouteConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;

  const SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SettingsComputer();
  const RoutePlannerConfig &route_planner =
    settings_computer.task.route_planner;

  static gcc_constexpr_data StaticEnumChoice final_glide_terrain_list[] = {
    { SETTINGS_FEATURES::FGT_OFF, N_("Off") },
    { SETTINGS_FEATURES::FGT_LINE, N_("Line") },
    { SETTINGS_FEATURES::FGT_SHADE, N_("Shade") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpFinalGlideTerrain"), final_glide_terrain_list,
                   settings_computer.FinalGlideTerrain);

  static gcc_constexpr_data StaticEnumChoice route_mode_list[] = {
    { RoutePlannerConfig::rpNone, N_("None") },
    { RoutePlannerConfig::rpTerrain, N_("Terrain") },
    { RoutePlannerConfig::rpAirspace, N_("Airspace") },
    { RoutePlannerConfig::rpBoth, N_("Both") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpRoutePlannerMode"), route_mode_list,
                   route_planner.mode);

  LoadFormProperty(*wf, _T("prpRoutePlannerAllowClimb"),
                   route_planner.allow_climb);

  LoadFormProperty(*wf, _T("prpRoutePlannerUseCeiling"),
                   route_planner.use_ceiling);

  static gcc_constexpr_data StaticEnumChoice turning_reach_list[] = {
    { RoutePlannerConfig::rmOff, N_("Off") },
    { RoutePlannerConfig::rmStraight, N_("Straight") },
    { RoutePlannerConfig::rmTurning, N_("Turning") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpTurningReach"), turning_reach_list,
                   route_planner.reach_calc_mode);

  static gcc_constexpr_data StaticEnumChoice reach_polar_list[] = {
    { RoutePlannerConfig::rpmTask, N_("Task") },
    { RoutePlannerConfig::rpmSafety, N_("Safety MC") },
    { 0 }
  };

  LoadFormProperty(*wf, _T("prpReachPolarMode"), reach_polar_list,
                   route_planner.reach_polar_mode);

  ShowRouteControls(route_planner.mode != RoutePlannerConfig::rpNone);
  ShowReachControls(route_planner.reach_calc_mode != RoutePlannerConfig::rmOff);
}


bool
RouteConfigPanel::Save()
{
  bool changed = false;
  SETTINGS_COMPUTER &settings_computer = XCSoarInterface::SetSettingsComputer();
  RoutePlannerConfig &route_planner = settings_computer.task.route_planner;

  changed |= SaveFormPropertyEnum(*wf, _T("prpRoutePlannerMode"),
                                  szProfileRoutePlannerMode,
                                  route_planner.mode);

  changed |= SaveFormPropertyEnum(*wf, _T("prpReachPolarMode"),
                                  szProfileReachPolarMode,
                                  route_planner.reach_polar_mode);

  changed |= SaveFormPropertyEnum(*wf, _T("prpFinalGlideTerrain"),
                                  szProfileFinalGlideTerrain,
                                  settings_computer.FinalGlideTerrain);

  changed |= SaveFormProperty(*wf, _T("prpRoutePlannerAllowClimb"),
                              szProfileRoutePlannerAllowClimb,
                              route_planner.allow_climb);

  changed |= SaveFormProperty(*wf, _T("prpRoutePlannerUseCeiling"),
                              szProfileRoutePlannerUseCeiling,
                              route_planner.use_ceiling);

  changed |= SaveFormPropertyEnum(*wf, _T("prpTurningReach"),
                                  szProfileTurningReach,
                                  route_planner.reach_calc_mode);

  return changed;
}
