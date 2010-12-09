/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef CROSS_SECTION_WINDOW_HPP
#define CROSS_SECTION_WINDOW_HPP

#include "Screen/PaintWindow.hpp"
#include "Blackboard.hpp"
#include "SettingsMapBlackboard.hpp"

class Airspaces;
class RasterTerrain;
class Chart;

/**
 * A Window which renders a terrain and airspace cross-section
 */
class CrossSectionWindow :
  public PaintWindow,
  public BaseBlackboard,
  public SettingsMapBlackboard
{
public:
  CrossSectionWindow();

  void ReadBlackboard(const NMEA_INFO &_gps_info,
                      const DERIVED_INFO &_calculated_info,
                      const SETTINGS_MAP &_settings_map);

  void Paint(Canvas &canvas, const RECT rc);

  void set_airspaces(Airspaces *_airspace_database) {
    airspace_database = _airspace_database;
  }
  void set_terrain(RasterTerrain *_terrain) {
    terrain = _terrain;
  }

  void set_range(fixed range) {
    vec.Distance = range;
  }
  void set_direction(Angle bearing) {
    vec.Bearing = bearing;
  }
  void set_start(GeoPoint _start) {
    start = _start;
  }

  void set_background_color(Color color) {
    background_color = color;
  }
  void set_text_color(Color color) {
    text_color = color;
  }
  void set_terrain_color(Color color) {
    terrain_brush.set(color);
  }

protected:
  void PaintAirspaces(Canvas &canvas, Chart &chart);
  void PaintTerrain(Canvas &canvas, Chart &chart);
  void PaintGlide(Chart &chart);
  void PaintAircraft(Canvas &canvas, const Chart &chart, const RECT rc);
  void PaintGrid(Canvas &canvas, Chart &chart);

  virtual void on_paint(Canvas &canvas);
  virtual bool on_create();

  Brush terrain_brush;

  RasterTerrain *terrain;
  Airspaces *airspace_database;

  GeoPoint start;
  GeoVector vec;

  Color background_color, text_color;
};

#endif
