/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/TextInBox.hpp"

enum DisplayTextType_t {
  DISPLAYNAME = 0,
  OBSOLETE_DONT_USE_DISPLAYNUMBER,
  DISPLAYFIRSTFIVE,
  DISPLAYNONE,
  DISPLAYFIRSTTHREE,
  OBSOLETE_DONT_USE_DISPLAYNAMEIFINTASK,
  DISPLAYUNTILSPACE
};

enum WaypointArrivalHeightDisplay_t {
  WP_ARRIVAL_HEIGHT_NONE = 0,
  WP_ARRIVAL_HEIGHT_GLIDE,
  WP_ARRIVAL_HEIGHT_TERRAIN,
  WP_ARRIVAL_HEIGHT_GLIDE_AND_TERRAIN
};

enum WaypointLabelSelection_t {
  wlsAllWaypoints,
  wlsTaskAndLandableWaypoints,
  wlsTaskWaypoints,
  wlsNoWaypoints
};

struct WaypointRendererSettings {
  /** What type of text to draw next to the waypoint icon */
  DisplayTextType_t display_text_type;

  /** Which arrival height to display next to waypoint labels */
  WaypointArrivalHeightDisplay_t arrival_height_display;

  /** What type of waypoint labels to render */
  WaypointLabelSelection_t label_selection;

  /** What type of waypoint labels to render */
  RenderMode landable_render_mode;

  void SetDefaults() {
    display_text_type = DISPLAYFIRSTFIVE;
    arrival_height_display = WP_ARRIVAL_HEIGHT_GLIDE;
    landable_render_mode = RoundedBlack;
  }

  void LoadFromProfile();
};

#endif
