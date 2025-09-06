// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RouteConfigPanel.hpp"
#include "Profile/Keys.hpp"
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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
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
RouteConfigPanel::OnModified(DataField &df) noexcept
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
RouteConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const RoutePlannerConfig &route_planner = settings_computer.task.route_planner;

  RowFormWidget::Prepare(parent, rc);

  static constexpr StaticEnumChoice route_mode_list[] = {
    { RoutePlannerConfig::Mode::NONE, N_("None"),
      N_("Neither airspace nor terrain is used for route planning.") },
    { RoutePlannerConfig::Mode::TERRAIN, N_("Terrain"),
      N_("Routes will avoid terrain.") },
    { RoutePlannerConfig::Mode::AIRSPACE, N_("Airspace"),
      N_("Routes will avoid airspace.") },
    { RoutePlannerConfig::Mode::BOTH, N_("Both"),
      N_("Routes will avoid airspace and terrain.") },
    nullptr
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
                 "current aircraft altitude plus 500 m and the thermal ceiling. If disabled, "
                 "climbs are unlimited."),
             route_planner.use_ceiling);
  SetExpertRow(RoutePlannerUseCeiling);

  static constexpr StaticEnumChoice turning_reach_list[] = {
    { RoutePlannerConfig::ReachMode::OFF, N_("Off"),
      N_("Reach calculations disabled.") },
    { RoutePlannerConfig::ReachMode::STRAIGHT, N_("Straight"),
      N_("The reach is from straight line paths from the glider.") },
    { RoutePlannerConfig::ReachMode::TURNING, N_("Turning"),
      N_("The reach is calculated allowing turns around terrain obstacles.") },
    nullptr
  };

  AddSpacer(); // Spacer

  AddEnum(_("Reach mode"),
          _("How calculations are performed of the reach of the glider with respect to terrain."),
          turning_reach_list, (unsigned)route_planner.reach_calc_mode,
          this);

  static constexpr StaticEnumChoice reach_polar_list[] = {
    { RoutePlannerConfig::Polar::TASK, N_("Task"),
      N_("Uses task glide polar.") },
    { RoutePlannerConfig::Polar::SAFETY, N_("Safety MC"),
      N_("Uses safety MacCready value.") },
    nullptr
  };

  AddEnum(_("Reach polar"),
          _("This determines the glide performance used in reach, landable arrival, abort and alternate calculations."),
          reach_polar_list, (unsigned)route_planner.reach_polar_mode);
  SetExpertRow(ReachPolarMode);

  static constexpr StaticEnumChoice final_glide_terrain_list[] = {
    { FeaturesSettings::FinalGlideTerrain::OFF, N_("Off"),
      N_("Disables the reach display.") },
    { FeaturesSettings::FinalGlideTerrain::TERRAIN_LINE, N_("Terrain line"),
      N_("Draws a dashed line at the terrain glide reach.") },
    { FeaturesSettings::FinalGlideTerrain::TERRAIN_SHADE, N_("Terrain shade"),
      N_("Shades terrain outside glide reach.") },
    { FeaturesSettings::FinalGlideTerrain::WORKING, N_("Working line"),
      N_("Draws a dashed line at the working glide reach.") },
    { FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_LINE, N_("Working line, terrain line"),
      N_("Draws a dashed line at the working and terrain glide reaches.") },
    { FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_SHADE, N_("Working line, terrain shade"),
      N_("Draws a dashed line at working, and shade terrain, glide reaches.") },
    nullptr
  };

  AddEnum(_("Reach display"), nullptr, final_glide_terrain_list,
          (unsigned)settings_computer.features.final_glide_terrain);

  ShowRouteControls(route_planner.mode != RoutePlannerConfig::Mode::NONE);
  ShowReachControls(route_planner.reach_calc_mode != RoutePlannerConfig::ReachMode::OFF);
}

bool
RouteConfigPanel::Save(bool &_changed) noexcept
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

std::unique_ptr<Widget>
CreateRouteConfigPanel()
{
  return std::make_unique<RouteConfigPanel>();
}
