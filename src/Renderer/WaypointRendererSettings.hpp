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

#ifndef XCSOAR_WAYPOINT_RENDERER_SETTINGS_HPP
#define XCSOAR_WAYPOINT_RENDERER_SETTINGS_HPP

#include "LabelShape.hpp"

#include <stdint.h>

struct WaypointRendererSettings {
  /** What type of text to draw next to the waypoint icon */
  enum class DisplayTextType : uint8_t {
    NAME = 0,
    OBSOLETE_DONT_USE_NUMBER,
    FIRST_FIVE,
    NONE,
    FIRST_THREE,
    OBSOLETE_DONT_USE_NAMEIFINTASK,
    FIRST_WORD,
  } display_text_type;

  /** Which arrival height to display next to waypoint labels */

  enum class ArrivalHeightDisplay : uint8_t {
    NONE = 0,
    GLIDE,
    TERRAIN,
    GLIDE_AND_TERRAIN,
    REQUIRED_GR,
  } arrival_height_display;

  /** What type of waypoint labels to render */
  enum class LabelSelection : uint8_t {
    ALL,
    TASK_AND_LANDABLE,
    TASK,
    NONE,
    TASK_AND_AIRFIELD,
  } label_selection;

  /** What type of waypoint labels to render */
  LabelShape landable_render_mode;

  enum class LandableStyle : uint8_t {
    PURPLE_CIRCLE,
    BW,
    TRAFFIC_LIGHTS,
  } landable_style;

  bool vector_landable_rendering;

  bool scale_runway_length;

  int landable_rendering_scale;

  void SetDefaults() {
    display_text_type = DisplayTextType::FIRST_FIVE;
    arrival_height_display = ArrivalHeightDisplay::GLIDE;
    label_selection = LabelSelection::ALL;
    landable_render_mode = LabelShape::ROUNDED_BLACK;

    landable_style = LandableStyle::PURPLE_CIRCLE;
    vector_landable_rendering = true;
    scale_runway_length = false;
    landable_rendering_scale = 100;
  }

  void LoadFromProfile();
};

#endif
