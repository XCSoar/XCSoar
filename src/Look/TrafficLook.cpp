// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficLook.hpp"
#include "Screen/Layout.hpp"
#include "Resources.hpp"

constexpr Color TrafficLook::team_color_green;
constexpr Color TrafficLook::team_color_magenta;
constexpr Color TrafficLook::team_color_blue;
constexpr Color TrafficLook::team_color_yellow;

void
TrafficLook::Initialise(const Font &_font)
{
  safe_above_brush.Create(safe_above_color);
  safe_below_brush.Create(safe_below_color);
  warning_brush.Create(warning_color);
  warning_in_altitude_range_brush.Create(warning_in_altitude_range_color);
  alarm_brush.Create(alarm_color);

  fading_pen.Create(Pen::Style::DASH1, Layout::ScalePenWidth(1), fading_outline_color);

#ifdef ENABLE_OPENGL
  fading_brush.Create(fading_fill_color);
#endif

  unsigned width = Layout::ScalePenWidth(2);
  team_pen_green.Create(width, team_color_green);
  team_pen_blue.Create(width, team_color_blue);
  team_pen_yellow.Create(width, team_color_yellow);
  team_pen_magenta.Create(width, team_color_magenta);

  teammate_icon.LoadResource(IDB_TEAMMATE_POS_ALL);

  font = &_font;
}
