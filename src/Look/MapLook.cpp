/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "MapLook.hpp"
#include "MapSettings.hpp"
#include "Screen/Layout.hpp"
#include "resource.h"

void
MapLook::Initialise(const MapSettings &settings,
                    const Font &font, const Font &bold_font)
{
  waypoint.Initialise(settings.waypoint, font, bold_font);
  airspace.Initialise(settings.airspace, font);
  aircraft.Initialise();
  task.Initialise();
  marker.Initialise();
  trail.Initialise(settings.trail);
  wind.Initialise(bold_font);

#ifdef HAVE_NOAA
  noaa.Initialise();
#endif

#ifdef HAVE_HATCHED_BRUSH
  above_terrain_bitmap.Load(IDB_ABOVETERRAIN);
  above_terrain_brush.Set(above_terrain_bitmap);
#endif

  terrain_warning_icon.LoadResource(IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD);

  compass_brush.Set(Color(207, 207, 207));
  compass_pen.Set(Layout::ScalePenWidth(1), HasColors()? COLOR_GRAY : COLOR_BLACK);
  compass_triangle_brush.Set(Color(50, 50, 50));
  compass_triangle_pen.Set(Layout::ScalePenWidth(1), HasColors()? COLOR_GRAY : COLOR_BLACK);

  traffic_safe_icon.LoadResource(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  traffic_warning_icon.LoadResource(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  traffic_alarm_icon.LoadResource(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  static constexpr Color clrSepia(0x78,0x31,0x18);
  reach_pen.Set(Pen::DASH, Layout::ScalePenWidth(1), clrSepia);
  reach_pen_thick.Set(Pen::DASH, Layout::ScalePenWidth(2), clrSepia);

  track_line_pen.Set(3, COLOR_GRAY);

  contest_pens[0].Set(Layout::ScalePenWidth(1) + 2, COLOR_RED);
  contest_pens[1].Set(Layout::ScalePenWidth(1) + 1, COLOR_ORANGE);
  contest_pens[2].Set(Layout::ScalePenWidth(1), COLOR_BLUE);

  thermal_source_icon.LoadResource(IDB_THERMALSOURCE, IDB_THERMALSOURCE_HD);

  traffic_safe_icon.LoadResource(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  traffic_warning_icon.LoadResource(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  traffic_alarm_icon.LoadResource(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  map_scale_left_icon.LoadResource(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  map_scale_right_icon.LoadResource(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);

  logger_on_icon.LoadResource(IDB_LOGGER, IDB_LOGGER_HD);
  logger_off_icon.LoadResource(IDB_LOGGEROFF, IDB_LOGGEROFF_HD);

  cruise_mode_icon.LoadResource(IDB_CRUISE, IDB_CRUISE_HD, false);
  climb_mode_icon.LoadResource(IDB_CLIMB, IDB_CLIMB_HD, false);
  final_glide_mode_icon.LoadResource(IDB_FINALGLIDE, IDB_FINALGLIDE_HD, false);
  abort_mode_icon.LoadResource(IDB_ABORT, IDB_ABORT_HD, false);

  waiting_for_fix_icon.LoadResource(IDB_GPSSTATUS1, IDB_GPSSTATUS1_HD, false);
  no_gps_icon.LoadResource(IDB_GPSSTATUS2, IDB_GPSSTATUS2_HD, false);

  overlay_font = &bold_font;
}
