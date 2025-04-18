// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Icon.hpp"
#include "ui/canvas/Brush.hpp"

class Font;
struct WaypointRendererSettings;

struct WaypointLook {
  MaskedIcon small_icon, turn_point_icon, task_turn_point_icon;
  MaskedIcon mountain_pass_icon;
  MaskedIcon mountain_top_icon, bridge_icon, obstacle_icon;
  MaskedIcon tower_icon, power_plant_icon, tunnel_icon, thermal_hotspot_icon;
  MaskedIcon marker_icon;
  MaskedIcon vor_icon, ndb_icon, reporting_point_icon;
  MaskedIcon dam_icon, castle_icon, intersection_icon;
  MaskedIcon pgtakeoff_icon, pglanding_icon;
  MaskedIcon airport_reachable_icon, airport_unreachable_icon;
  MaskedIcon airport_marginal_icon;
  MaskedIcon field_marginal_icon, field_reachable_icon, field_unreachable_icon;

  Brush reachable_brush, terrain_unreachable_brush, unreachable_brush;

  Brush white_brush, light_gray_brush, magenta_brush, orange_brush;

  const Font *font, *bold_font;

  void Initialise(const WaypointRendererSettings &settings,
                  const Font &font, const Font &bold_font);

  void Reinitialise(const WaypointRendererSettings &settings);
};
