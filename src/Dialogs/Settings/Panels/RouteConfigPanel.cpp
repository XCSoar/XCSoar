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
    N_("Disables map reach (terrain line and shade). Abort, "
       "alternates and landable arrival still use the Reach polar.") },
  { RoutePlannerConfig::ReachMode::STRAIGHT, N_("Straight"),
    N_("The reach is from straight line paths from the glider. "
       "Landable colours and the Alternates list use this reach.") },
  { RoutePlannerConfig::ReachMode::TURNING, N_("Turning"),
    N_("The reach is calculated allowing turns around terrain obstacles. "
       "Landable colours and the Alternates list use this reach.") },
  nullptr
};

static constexpr StaticEnumChoice reach_polar_list[] = {
  { RoutePlannerConfig::Polar::TASK, N_("Task MC"),
    N_("Uses task MacCready value.") },
  { RoutePlannerConfig::Polar::SAFETY, N_("Safety MC"),
    N_("Uses safety MacCready value.") },
  nullptr
};

static constexpr StaticEnumChoice final_glide_terrain_list[] = {
  { FeaturesSettings::FinalGlideTerrain::OFF, N_("Off"),
    N_("No reach overlay on the map.") },
  { FeaturesSettings::FinalGlideTerrain::TERRAIN_LINE, N_("Terrain line"),
    N_("How far you can glide to the ground. Uses the Reach polar.") },
  { FeaturesSettings::FinalGlideTerrain::TERRAIN_SHADE, N_("Terrain shade"),
    N_("Shades the map beyond glide to the ground. Uses the Reach polar.") },
  { FeaturesSettings::FinalGlideTerrain::WORKING, N_("Working line"),
    N_("How far you can glide and still arrive at typical thermal-base "
       "height from this flight. Always uses Task MC.") },
  { FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_LINE,
    N_("Working line, terrain line"),
    N_("Working line to typical thermal-base height using Task MC, plus "
       "terrain line to the ground using the Reach polar.") },
  { FeaturesSettings::FinalGlideTerrain::WORKING_TERRAIN_SHADE,
    N_("Working line, terrain shade"),
    N_("Working line to typical thermal-base height using Task MC, plus "
       "shade beyond the ground using the Reach polar.") },
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

  if (IsExpert())
    AddEnumItem(_("Reach polar"),
                _("Which MacCready is used for terrain reach, landable "
                  "colours, abort and the Alternates list. This still "
                  "applies when Reach mode is Off. The working line "
                  "always uses Task MC. Alternate InfoBoxes and waypoint "
                  "Alt. diff. Safety MC always use Safety MC."),
                reach_polar_list, reach_polar);

  /* the overlay is the map reach; abort still uses the polar */
  if (reach_mode != RoutePlannerConfig::ReachMode::OFF)
    AddEnumItem(_("Reach display"), nullptr, final_glide_terrain_list,
                final_glide_terrain);
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
