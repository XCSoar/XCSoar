// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficLook.hpp"
#include "Screen/Layout.hpp"
#include "Resources.hpp"
#include "FLARM/Traffic.hpp"

constexpr Color TrafficLook::team_color_green;
constexpr Color TrafficLook::team_color_magenta;
constexpr Color TrafficLook::team_color_blue;
constexpr Color TrafficLook::team_color_yellow;
constexpr Color TrafficLook::symbol_glyph_color;
constexpr Color TrafficLook::symbol_halo_color;

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

  symbol_border_pen.Create(Layout::ScalePenWidth(1), symbol_glyph_color);

  teammate_icon.LoadResource(IDB_TEAMMATE_POS_ALL);

  font = &_font;
}

Color
TrafficLook::GetBodyColor(const FlarmTraffic &traffic) const noexcept
{
  switch (traffic.alarm_level) {
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    return warning_color;

  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    return alarm_color;

  case FlarmTraffic::AlarmType::NONE:
    break;
  }

  if (traffic.relative_altitude > (const RoughAltitude)50)
    return safe_above_color;
  else if (traffic.relative_altitude > (const RoughAltitude)-50)
    return warning_in_altitude_range_color;
  else
    return safe_below_color;
}

const Brush &
TrafficLook::GetBodyBrush(const FlarmTraffic &traffic) const noexcept
{
  switch (traffic.alarm_level) {
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    return warning_brush;

  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    return alarm_brush;

  case FlarmTraffic::AlarmType::NONE:
    break;
  }

  if (traffic.relative_altitude > (const RoughAltitude)50)
    return safe_above_brush;
  else if (traffic.relative_altitude > (const RoughAltitude)-50)
    return warning_in_altitude_range_brush;
  else
    return safe_below_brush;
}
