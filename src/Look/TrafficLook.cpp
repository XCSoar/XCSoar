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
  colorful_traffic_brushes.above.climb_good.Create(colorful_traffic_colors::above::climb_good);
  colorful_traffic_brushes.above.climb_up.Create(colorful_traffic_colors::above::climb_up);
  colorful_traffic_brushes.above.climb_down.Create(colorful_traffic_colors::above::climb_down);

  colorful_traffic_brushes.same.climb_good.Create(colorful_traffic_colors::same::climb_good);
  colorful_traffic_brushes.same.climb_up.Create(colorful_traffic_colors::same::climb_up);
  colorful_traffic_brushes.same.climb_down.Create(colorful_traffic_colors::same::climb_down);

  colorful_traffic_brushes.below.climb_good.Create(colorful_traffic_colors::below::climb_good);
  colorful_traffic_brushes.below.climb_up.Create(colorful_traffic_colors::below::climb_up);
  colorful_traffic_brushes.below.climb_down.Create(colorful_traffic_colors::below::climb_down);

  basic_traffic_brushes.above.Create(TrafficLookColor::above);
  basic_traffic_brushes.same.Create(TrafficLookColor::same);
  basic_traffic_brushes.below.Create(TrafficLookColor::below);

  warning_brush.Create(warning_color);
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

const Brush& TrafficLook::GetBasicTrafficBrush(const TrafficClimbAltIndicators &indicators) const noexcept
{
  switch (indicators.get_rel_alt_indicator()) 
  {
    case (int)TrafficClimbAltIndicators::RelAlt::ABOVE:
      return basic_traffic_brushes.above;
    case (int)TrafficClimbAltIndicators::RelAlt::SAME:
      return basic_traffic_brushes.same;
    case (int)TrafficClimbAltIndicators::RelAlt::BELOW:
      return basic_traffic_brushes.below;
    default:
      return basic_traffic_brushes.same;
  }
}

const Brush& TrafficLook::GetColourfulTrafficBrush(const TrafficClimbAltIndicators &indicators) const noexcept
{
  auto rel_alt_indicator = indicators.get_rel_alt_indicator();
  auto climb_indicator = indicators.get_climb_indicator();

  const Climb_Indication_t& climbBrushes = 
      (rel_alt_indicator == (int)TrafficClimbAltIndicators::RelAlt::ABOVE) ? colorful_traffic_brushes.above :
      (rel_alt_indicator == (int)TrafficClimbAltIndicators::RelAlt::SAME) ? colorful_traffic_brushes.same :
      (rel_alt_indicator == (int)TrafficClimbAltIndicators::RelAlt::BELOW) ? colorful_traffic_brushes.below :
      colorful_traffic_brushes.same; // default value

  return (climb_indicator == (int)TrafficClimbAltIndicators::Climb::GOOD) ? climbBrushes.climb_good :
          (climb_indicator == (int)TrafficClimbAltIndicators::Climb::UP) ? climbBrushes.climb_up :
          (climb_indicator == (int)TrafficClimbAltIndicators::Climb::DOWN) ? climbBrushes.climb_down :
          climbBrushes.climb_good; // default value
}
