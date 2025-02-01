// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"

struct TrafficLook;

struct FlarmTrafficLook {
  Color warning_color;
  Color alarm_color;
  Color default_color;
  Color passive_color;
  Color selection_color;
  Color background_color;
  Color radar_color;
  Color safe_above_color;
  Color safe_below_color;
  Color warning_in_altitude_range_color;

  Brush warning_brush;
  Brush alarm_brush;
  Brush default_brush;
  Brush passive_brush;
  Brush selection_brush;
  Brush radar_brush;
  Brush team_brush_green;
  Brush team_brush_blue;
  Brush team_brush_yellow;
  Brush team_brush_magenta;
  Brush safe_above_brush;
  Brush safe_below_brush;
  Brush warning_in_altitude_range_brush;

  Pen warning_pen;
  Pen alarm_pen;
  Pen default_pen;
  Pen passive_pen;
  Pen selection_pen;

  Pen team_pen_green;
  Pen team_pen_blue;
  Pen team_pen_yellow;
  Pen team_pen_magenta;

  Pen plane_pen, radar_pen;

  Pen unit_fraction_pen;

  Font label_font, side_info_font, no_traffic_font;
  Font info_values_font, info_units_font, info_labels_font, call_sign_font;

  void Initialise(const TrafficLook &other, bool small, bool inverse = false);
};
