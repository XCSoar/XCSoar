// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointLook.hpp"
#include "AirspaceLook.hpp"
#include "AircraftLook.hpp"
#include "TaskLook.hpp"
#include "TrailLook.hpp"
#include "WaveLook.hpp"
#include "WindArrowLook.hpp"
#include "TopographyLook.hpp"
#include "OverlayLook.hpp"
#include "ui/canvas/Icon.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Features.hpp"
#include "Weather/Features.hpp"

#ifdef HAVE_NOAA
#include "NOAALook.hpp"
#endif

struct MapSettings;

struct MapLook {
  WaypointLook waypoint;
  AirspaceLook airspace;
  AircraftLook aircraft;
  TaskLook task;
  TrailLook trail;
  WaveLook wave;
  WindArrowLook wind;

#ifdef HAVE_NOAA
  NOAALook noaa;
#endif

  OverlayLook overlay;

#ifdef HAVE_HATCHED_BRUSH
  Bitmap above_terrain_bitmap;
  Brush above_terrain_brush;
#endif

  MaskedIcon terrain_warning_icon;

  Pen compass_pen;
  Brush compass_brush;
  Pen compass_triangle_pen;
  Brush compass_triangle_brush;

  Pen reach_terrain_pen;
  Pen reach_terrain_pen_thick;

  Pen reach_working_pen;
  Pen reach_working_pen_thick;

  Pen track_line_pen;

  Pen contest_pens[3];

  MaskedIcon thermal_source_icon;

  MaskedIcon traffic_safe_icon;
  MaskedIcon traffic_warning_icon;
  MaskedIcon traffic_alarm_icon;

  MaskedIcon network_connected_icon;
  MaskedIcon network_disconnected_icon;

  MaskedIcon cruise_mode_icon, climb_mode_icon, final_glide_mode_icon, abort_mode_icon;
  MaskedIcon waiting_for_fix_icon, no_gps_icon;

  TopographyLook topography;

  void Initialise(const MapSettings &settings,
                  const Font &font, const Font &bold_font);
};
