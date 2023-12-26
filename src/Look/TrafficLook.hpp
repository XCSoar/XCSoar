// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Icon.hpp"
#include "FLARM/TrafficClimbAltIndicators.hpp"

class Font;

struct TrafficLook
{
  struct colorful_traffic_colors
  {
    struct above
    {
      static constexpr Color climb_good = {0xff, 0x66, 0x66}; // light red
      static constexpr Color climb_up = {0xff, 0xff, 0x66};   // light yellow
      static constexpr Color climb_down = {0x66, 0x66, 0xff}; // light blue
    };

    struct same
    {
      static constexpr Color climb_good = {0xff, 0x00, 0x00}; // red
      static constexpr Color climb_up = {0xff, 0xff, 0x00};   // yellow
      static constexpr Color climb_down = {0x00, 0x00, 0xff}; // blue
    };

    struct below
    {
      static constexpr Color climb_good = {0x99, 0x00, 0x00}; // dark red
      static constexpr Color climb_up = {0x99, 0x99, 0x00};   // dark yellow
      static constexpr Color climb_down = {0x00, 0x00, 0x99}; // dark blue
    };
  };

  struct TrafficLookColor
  {
    static constexpr Color above = {0x1d, 0x9b, 0xc5};
    static constexpr Color same = {0xff, 0x00, 0xff};
    static constexpr Color below = {0x1d, 0xc5, 0x10};
  };

  static constexpr Color warning_color{0xfe, 0x84, 0x38};
  static constexpr Color warning_in_altitude_range_color{0xff, 0x00, 0xff};
  static constexpr Color alarm_color{0xfb, 0x35, 0x2f};

  struct basic_traffic_brushes
  {
    Brush above;
    Brush same;
    Brush below;
  } basic_traffic_brushes;

  typedef struct Climb_Indication_s {
      Brush climb_good;
      Brush climb_up;
      Brush climb_down;
  }Climb_Indication_t;

  struct colorful_traffic_brushes
  {
    Climb_Indication_t above;
    Climb_Indication_t same;
    Climb_Indication_t below;
  } colorful_traffic_brushes;

  Brush warning_brush;
  Brush alarm_brush;

  static constexpr Color fading_outline_color = ColorWithAlpha({0x60, 0x60, 0x60}, 0xa0);
  Pen fading_pen;

#ifdef ENABLE_OPENGL
  static constexpr Color fading_fill_color = ColorWithAlpha({0xc0, 0xc0, 0xc0}, 0x60);
  Brush fading_brush;
#endif

  static constexpr Color team_color_green = Color(0x74, 0xff, 0);
  static constexpr Color team_color_magenta = Color(0xff, 0, 0xcb);
  static constexpr Color team_color_blue = Color(0, 0x90, 0xff);
  static constexpr Color team_color_yellow = Color(0xff, 0xe8, 0);

  Pen team_pen_green;
  Pen team_pen_blue;
  Pen team_pen_yellow;
  Pen team_pen_magenta;

  MaskedIcon teammate_icon;

  const Font *font;

  void Initialise(const Font &font);

  const Brush& GetBasicTrafficBrush(const TrafficClimbAltIndicators &indicators) const noexcept;
  const Brush& GetColourfulTrafficBrush(const TrafficClimbAltIndicators &indicators) const noexcept;
};
