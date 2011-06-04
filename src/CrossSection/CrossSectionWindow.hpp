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

#ifndef CROSS_SECTION_WINDOW_HPP
#define CROSS_SECTION_WINDOW_HPP

#include "Screen/PaintWindow.hpp"
#include "Blackboard.hpp"
#include "SettingsMapBlackboard.hpp"

struct CrossSectionLook;
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
protected:
  const CrossSectionLook &look;

  /** Pointer to a RasterTerrain instance or NULL */
  const RasterTerrain *terrain;

  /** Pointer to an airspace database instance or NULL */
  const Airspaces *airspace_database;

  /** Left side of the CrossSectionWindow */
  GeoPoint start;
  /** Range and direction of the CrossSection */
  GeoVector vec;

public:
  /**
   * Constructor. Initializes most class members.
   */
  CrossSectionWindow(const CrossSectionLook &look);

  void ReadBlackboard(const NMEA_INFO &_gps_info,
                      const DERIVED_INFO &_calculated_info,
                      const SETTINGS_MAP &_settings_map);

  /**
   * Renders the CrossSection to the given canvas in the given PixelRect
   * @param canvas Canvas to draw on
   * @param rc PixelRect to draw in
   */
  void Paint(Canvas &canvas, const PixelRect rc);

  /**
   * Set airspace database to use
   * @param _airspace_database Pointer to the airspace database or NULL
   */
  void set_airspaces(const Airspaces *_airspace_database) {
    airspace_database = _airspace_database;
  }

  /**
   * Set RasterTerrain to use
   * @param _terrain Pointer to the RasterTerrain or NULL
   */
  void set_terrain(const RasterTerrain *_terrain) {
    terrain = _terrain;
  }

  /**
   * Set CrossSection range
   * @param range Range to draw [m]
   */
  void set_range(fixed range) {
    vec.Distance = range;
  }
  /**
   * Set CrossSection direction
   * @param bearing Direction to draw
   */
  void set_direction(Angle bearing) {
    vec.Bearing = bearing;
  }
  /**
   * Set CrossSection start point
   * @param _start Start GeoPoint to use for drawing
   */
  void set_start(GeoPoint _start) {
    start = _start;
  }

protected:
  void PaintAirspaces(Canvas &canvas, Chart &chart);
  void PaintTerrain(Canvas &canvas, Chart &chart);
  void PaintGlide(Chart &chart);
  void PaintAircraft(Canvas &canvas, const Chart &chart, const PixelRect rc);
  void PaintGrid(Canvas &canvas, Chart &chart);

  /**
   * OnPaint event called by the message loop
   * @param canvas Canvas to draw to
   */
  virtual void on_paint(Canvas &canvas);
};

#endif
