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

#include "RouteConfigPanel.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/Base.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  RoutePlannerMode,
  RoutePlannerAllowClimb,
  RoutePlannerUseCeiling,
  empty_spacer,
  TurningReach,
  ReachPolarMode,
  FinalGlideTerrain,
};

class RouteConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  RouteConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void ShowRouteControls(bool show);
  void ShowReachControls(bool show);

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual bool Save(bool &changed) override;

private:
  /* methods from DataFieldListener */
  virtual void OnModified(DataField &df) override;
};

void
RouteConfigPanel::ShowRouteControls(bool show)
{
  SetRowVisible(RoutePlannerAllowClimb, show);
  SetRowVisible(RoutePlannerUseCeiling, show);
}

void
RouteConfigPanel::ShowReachControls(bool show)
{
  SetRowVisible(FinalGlideTerrain, show);
  SetRowVisible(ReachPolarMode, show);
}

void
RouteConfigPanel::OnModified(DataField &df)
{
  if (IsDataField(RoutePlannerMode, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    RoutePlannerConfig::Mode mode =
      (RoutePlannerConfig::Mode)dfe.GetValue();
    ShowRouteControls(mode != RoutePlannerConfig::Mode::NONE);
  } else if (IsDataField(TurningReach, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    RoutePlannerConfig::ReachMode mode =
      (RoutePlannerConfig::ReachMode)dfe.GetValue();
    ShowReachControls(mode != RoutePlannerConfig::ReachMode::OFF);
  }
}

void
RouteConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const RoutePlannerConfig &route_planner = settings_computer.task.route_planner;

  RowFormWidget::Prepare(parent, rc);

  static constexpr StaticEnumChoice route_mode_list[] = {
    { (unsigned)RoutePlannerConfig::Mode::NONE, N_("None"),
      N_("Neither airspace nor terrain is used for route planning.") },
    { (unsigned)RoutePlannerConfig::Mode::TERRAIN, N_("Terrain"),
      N_("Routes will avoid terrain.") },
    { (unsigned)RoutePlannerConfig::Mode::AIRSPACE, N_("Airspace"),
      N_("Routes will avoid airspace.") },
    { (unsigned)RoutePlannerConfig::Mode::BOTH, N_("Both"),
      N_("Routes will avoid airspace and terrain.") },
    { 0 }
  };

  AddEnum(_("Route mode"), nullptr, route_mode_list,
          (unsigned)route_planner.mode, this);

  AddBoolean(_("Route climb"),
             _("When enabled and MC is positive, route planning allows climbs between the aircraft "
                 "location and destination."),
             route_planner.allow_climb);
  SetExpertRow(RoutePlannerAllowClimb);

  AddBoolean(_("Route ceiling"),
             _("When enabled, route planning climbs are limited to ceiling defined by greater of "
                 "current aircraft altitude plus 500 m and the thermal ceiling.  If disabled, "
                 "climbs are unlimited."),
             route_planner.use_ceiling);
  SetExpertRow(RoutePlannerUseCeiling);

  static constexpr StaticEnumChoice turning_reach_list[] = {
    { (unsigned)RoutePlannerConfig::ReachMode::OFF, N_("Off"),
      N_("Reach calculations disabled.") },
    { (unsigned)RoutePlannerConfig::ReachMode::STRAIGHT, N_("Straight"),
      N_("The reach is from straight line paths from the glider.") },
    { (unsigned)RoutePlannerConfig::ReachMode::TURNING, N_("Turning"),
      N_("The reach is calculated allowing turns around terrain obstacles.") },
    { 0 }
  };

  AddSpacer(); // Spacer

  AddEnum(_("Reach mode"),
          _("How calculations are performed of the reach of the glider with respect to terrain."),
          turning_reach_list, (unsigned)route_planner.reach_calc_mode,
          this);

  static constexpr StaticEnumChoice reach_polar_list[] = {
    { (unsigned)RoutePlannerConfig::Polar::TASK, N_("Task"),
      N_("Uses task glide polar.") },
    { (unsigned)RoutePlannerConfig::Polar::SAFETY, N_("Safety MC"),
      N_("Uses safety MacCready value") },
    { 0 }
  };

  AddEnum(_("Reach polar"),
          _("This determines the glide performance used in reach, landable arrival, abort and alternate calculations."),
          reach_polar_list, (unsigned)route_planner.reach_polar_mode);
  SetExpertRow(ReachPolarMode);

  static constexpr StaticEnumChoice final_glide_terrain_list[] = {
    { (unsigned)FeaturesSettings::FinalGlideTerrain::OFF, N_("Off"),
      N_("Disables the reach display.") },
    { (unsigned)FeaturesSettings::FinalGlideTerrain::TERRAIN_LINE, N_("Terrain line"),
      N_("Draws a dashed line at the terrain glide reach.") },
    { (unsigned)FeaturesSettings::FinalGlideTerrain::TERRAIN_SHADE, N_("Terrain shade"),
      N_("Shades terrain outside glide reach.") },
    { (unsigned)FeaturesSettings::FinalGlideTerrain::WORKING, N_("Working line"),
      N_("Draws a dashed line at the working glide reach.") },
    { (unsigned)FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_LINE, N_("Working line, terrain line"),
      N_("Draws a dashed line at the working and terrain glide reaches.") },
    { (unsigned)FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_SHADE, N_("Working line, terrain shade"),
      N_("Draws a dashed line at working, and shade terrain, glide reaches.") },
    { 0 }
  };

  AddEnum(_("Reach display"), nullptr, final_glide_terrain_list,
          (unsigned)settings_computer.features.final_glide_terrain);

  ShowRouteControls(route_planner.mode != RoutePlannerConfig::Mode::NONE);
  ShowReachControls(route_planner.reach_calc_mode != RoutePlannerConfig::ReachMode::OFF);
}

bool
RouteConfigPanel::Save(bool &_changed)
{
  bool changed = false;
  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  RoutePlannerConfig &route_planner = settings_computer.task.route_planner;

  changed |= SaveValueEnum(RoutePlannerMode, ProfileKeys::RoutePlannerMode,
                           route_planner.mode);

  changed |= SaveValueEnum(ReachPolarMode, ProfileKeys::ReachPolarMode,
                           route_planner.reach_polar_mode);

  changed |= SaveValueEnum(FinalGlideTerrain, ProfileKeys::FinalGlideTerrain,
                           settings_computer.features.final_glide_terrain);

  changed |= SaveValue(RoutePlannerAllowClimb, ProfileKeys::RoutePlannerAllowClimb,
                       route_planner.allow_climb);

  changed |= SaveValue(RoutePlannerUseCeiling, ProfileKeys::RoutePlannerUseCeiling,
                       route_planner.use_ceiling);

  changed |= SaveValueEnum(TurningReach, ProfileKeys::TurningReach,
                           route_planner.reach_calc_mode);
  _changed |= changed;

  return true;
}

Widget *
CreateRouteConfigPanel()
{
  return new RouteConfigPanel();
}
