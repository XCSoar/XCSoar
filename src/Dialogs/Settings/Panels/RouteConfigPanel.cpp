// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RouteConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

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

static constexpr StaticEnumChoice turning_reach_list[] = {
  { RoutePlannerConfig::ReachMode::OFF, N_("Off"),
    N_("Reach calculations disabled.") },
  { RoutePlannerConfig::ReachMode::STRAIGHT, N_("Straight"),
    N_("The reach is from straight line paths from the glider.") },
  { RoutePlannerConfig::ReachMode::TURNING, N_("Turning"),
    N_("The reach is calculated allowing turns around terrain obstacles.") },
  nullptr
};

static constexpr StaticEnumChoice reach_polar_list[] = {
  { RoutePlannerConfig::Polar::TASK, N_("Task"),
    N_("Uses task glide polar.") },
  { RoutePlannerConfig::Polar::SAFETY, N_("Safety MC"),
    N_("Uses safety MacCready value.") },
  nullptr
};

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

/**
 * The route planner and the reach: what each avoids, and what the
 * reach is drawn with.
 */
class RouteConfigPanel final : public ConfigListPanel {
  RoutePlannerConfig::Mode route_mode;
  bool allow_climb, use_ceiling;

  RoutePlannerConfig::ReachMode reach_mode;
  RoutePlannerConfig::Polar reach_polar;
  FeaturesSettings::FinalGlideTerrain final_glide_terrain;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
RouteConfigPanel::LoadSettings() noexcept
{
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();
  const RoutePlannerConfig &route_planner =
    settings_computer.task.route_planner;

  route_mode = route_planner.mode;
  allow_climb = route_planner.allow_climb;
  use_ceiling = route_planner.use_ceiling;

  reach_mode = route_planner.reach_calc_mode;
  reach_polar = route_planner.reach_polar_mode;
  final_glide_terrain = settings_computer.features.final_glide_terrain;
}

void
RouteConfigPanel::Fill() noexcept
{
  AddGroup(_("Route"));

  AddEnumItem(_("Route mode"), nullptr, route_mode_list, route_mode);

  if (route_mode != RoutePlannerConfig::Mode::NONE && IsExpert()) {
    AddToggleItem(_("Route climb"),
                  _("When enabled and MC is positive, route planning allows climbs between the aircraft "
                    "location and destination."),
                  allow_climb);

    AddToggleItem(_("Route ceiling"),
                  _("When enabled, route planning climbs are limited to ceiling defined by greater of "
                    "current aircraft altitude plus 500 m and the thermal ceiling. If disabled, "
                    "climbs are unlimited."),
                  use_ceiling);
  }

  AddGroup(_("Reach"));

  AddEnumItem(_("Reach mode"),
              _("How calculations are performed of the reach of the glider with respect to terrain."),
              turning_reach_list, reach_mode);

  if (reach_mode != RoutePlannerConfig::ReachMode::OFF) {
    if (IsExpert())
      AddEnumItem(_("Reach polar"),
                  _("This determines the glide performance used in reach, landable arrival, abort and alternate calculations."),
                  reach_polar_list, reach_polar);

    AddEnumItem(_("Reach display"), nullptr, final_glide_terrain_list,
                final_glide_terrain);
  }
}

bool
RouteConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  RoutePlannerConfig &route_planner = settings_computer.task.route_planner;

  changed |= Profile::Update(ProfileKeys::RoutePlannerMode,
                             route_planner.mode, route_mode);

  changed |= Profile::Update(ProfileKeys::RoutePlannerAllowClimb,
                             route_planner.allow_climb, allow_climb);

  changed |= Profile::Update(ProfileKeys::RoutePlannerUseCeiling,
                             route_planner.use_ceiling, use_ceiling);

  changed |= Profile::Update(ProfileKeys::TurningReach,
                             route_planner.reach_calc_mode, reach_mode);

  changed |= Profile::Update(ProfileKeys::ReachPolarMode,
                             route_planner.reach_polar_mode, reach_polar);

  changed |= Profile::Update(ProfileKeys::FinalGlideTerrain,
                             settings_computer.features.final_glide_terrain,
                             final_glide_terrain);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateRouteConfigPanel()
{
  return std::make_unique<RouteConfigPanel>();
}
