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
#include "Form/Form.hpp"
#include "DataField/Base.hpp"
#include "Form/XMLWidget.hpp"
#include "Screen/Layout.hpp"
#include "Dialogs/CallBackTable.hpp"

class RouteConfigPanel : public XMLWidget {

public:
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void ShowRouteControls(bool show);
  void ShowReachControls(bool show);

};

/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static RouteConfigPanel *instance;

void
RouteConfigPanel::Show(const PixelRect &rc)
{
  XMLWidget::Show(rc);
}

void
RouteConfigPanel::Hide()
{
  XMLWidget::Hide();
}

void
RouteConfigPanel::ShowRouteControls(bool show)
{
  ShowFormControl(form, _T("prpRoutePlannerAllowClimb"), show);
  ShowFormControl(form, _T("prpRoutePlannerUseCeiling"), show);
}

void
RouteConfigPanel::ShowReachControls(bool show)
{
  ShowFormControl(form, _T("prpFinalGlideTerrain"), show);
  ShowFormControl(form, _T("prpReachPolarMode"), show);
}

static void
OnRouteMode(DataField *Sender,
            DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  RoutePlannerConfig::Mode mode = (RoutePlannerConfig::Mode)df.GetAsInteger();
  instance->ShowRouteControls(mode != RoutePlannerConfig::rpNone);
}

static void
OnReachMode(DataField *Sender,
            DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  RoutePlannerConfig::ReachMode mode =
    (RoutePlannerConfig::ReachMode)df.GetAsInteger();
  instance->ShowReachControls(mode != RoutePlannerConfig::rmOff);
}

gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnRouteMode),
  DeclareCallBackEntry(OnReachMode),
  DeclareCallBackEntry(NULL)
};

void
RouteConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  instance = this;
  LoadWindow(CallBackTable, parent,
             Layout::landscape ? _T("IDR_XML_ROUTECONFIGPANEL") :
                               _T("IDR_XML_ROUTECONFIGPANEL_L"));

  const ComputerSettings &settings_computer = XCSoarInterface::GetComputerSettings();
  const RoutePlannerConfig &route_planner =
    settings_computer.task.route_planner;

  static gcc_constexpr_data StaticEnumChoice final_glide_terrain_list[] = {
    { FeaturesSettings::FGT_OFF, N_("Off") },
    { FeaturesSettings::FGT_LINE, N_("Line") },
    { FeaturesSettings::FGT_SHADE, N_("Shade") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpFinalGlideTerrain"), final_glide_terrain_list,
                   settings_computer.final_glide_terrain);

  static gcc_constexpr_data StaticEnumChoice route_mode_list[] = {
    { RoutePlannerConfig::rpNone, N_("None") },
    { RoutePlannerConfig::rpTerrain, N_("Terrain") },
    { RoutePlannerConfig::rpAirspace, N_("Airspace") },
    { RoutePlannerConfig::rpBoth, N_("Both") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpRoutePlannerMode"), route_mode_list,
                   route_planner.mode);

  LoadFormProperty(form, _T("prpRoutePlannerAllowClimb"),
                   route_planner.allow_climb);

  LoadFormProperty(form, _T("prpRoutePlannerUseCeiling"),
                   route_planner.use_ceiling);

  static gcc_constexpr_data StaticEnumChoice turning_reach_list[] = {
    { RoutePlannerConfig::rmOff, N_("Off") },
    { RoutePlannerConfig::rmStraight, N_("Straight") },
    { RoutePlannerConfig::rmTurning, N_("Turning") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpTurningReach"), turning_reach_list,
                   route_planner.reach_calc_mode);

  static gcc_constexpr_data StaticEnumChoice reach_polar_list[] = {
    { RoutePlannerConfig::rpmTask, N_("Task") },
    { RoutePlannerConfig::rpmSafety, N_("Safety MC") },
    { 0 }
  };

  LoadFormProperty(form, _T("prpReachPolarMode"), reach_polar_list,
                   route_planner.reach_polar_mode);

  ShowRouteControls(route_planner.mode != RoutePlannerConfig::rpNone);
  ShowReachControls(route_planner.reach_calc_mode != RoutePlannerConfig::rmOff);
}

bool
RouteConfigPanel::Save(bool &_changed, bool &_require_restart)
{
  bool changed = false, require_restart = false;
  ComputerSettings &settings_computer = XCSoarInterface::SetComputerSettings();
  RoutePlannerConfig &route_planner = settings_computer.task.route_planner;

  changed |= SaveFormPropertyEnum(form, _T("prpRoutePlannerMode"),
                                  szProfileRoutePlannerMode,
                                  route_planner.mode);

  changed |= SaveFormPropertyEnum(form, _T("prpReachPolarMode"),
                                  szProfileReachPolarMode,
                                  route_planner.reach_polar_mode);

  changed |= SaveFormPropertyEnum(form, _T("prpFinalGlideTerrain"),
                                  szProfileFinalGlideTerrain,
                                  settings_computer.final_glide_terrain);

  changed |= SaveFormProperty(form, _T("prpRoutePlannerAllowClimb"),
                              szProfileRoutePlannerAllowClimb,
                              route_planner.allow_climb);

  changed |= SaveFormProperty(form, _T("prpRoutePlannerUseCeiling"),
                              szProfileRoutePlannerUseCeiling,
                              route_planner.use_ceiling);

  changed |= SaveFormPropertyEnum(form, _T("prpTurningReach"),
                                  szProfileTurningReach,
                                  route_planner.reach_calc_mode);
  _changed |= changed;
  _require_restart |= require_restart;

  return true;
}

Widget *
CreateRouteConfigPanel()
{
  return new RouteConfigPanel();
}
