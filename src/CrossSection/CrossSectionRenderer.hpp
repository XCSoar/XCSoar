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

#ifndef CROSS_SECTION_RENDERER_HPP
#define CROSS_SECTION_RENDERER_HPP

#include "Blackboard/BaseBlackboard.hpp"
#include "TerrainXSRenderer.hpp"
#include "AirspaceXSRenderer.hpp"
#include "Engine/GlideSolvers/GlideSettings.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"

struct PixelRect;
struct MoreData;
struct CrossSectionLook;
struct AirspaceLook;
struct ChartLook;
struct MapSettings;
class Airspaces;
class RasterTerrain;
class ChartRenderer;
class Canvas;

/**
 * A Window which renders a terrain and airspace cross-section
 */
class CrossSectionRenderer :
  public BaseBlackboard
{
public:
  static constexpr unsigned NUM_SLICES = 64;
  const bool inverse;

protected:
  const CrossSectionLook &look;
  const ChartLook &chart_look;

  GlideSettings glide_settings;
  GlidePolar glide_polar;

  AirspaceXSRenderer airspace_renderer;
  TerrainXSRenderer terrain_renderer;

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
  CrossSectionRenderer(const CrossSectionLook &look,
                       const AirspaceLook &airspace_look,
                       const ChartLook &chart_look,
                       const bool &_inverse);

  void ReadBlackboard(const MoreData &_gps_info,
                      const DerivedInfo &_calculated_info,
                      const GlideSettings &glide_settings,
                      const GlidePolar &glide_polar,
                      const MapSettings &map_settings);

  /**
   * Renders the CrossSection to the given canvas in the given PixelRect
   * @param canvas Canvas to draw on
   * @param rc PixelRect to draw in
   */
  void Paint(Canvas &canvas, const PixelRect rc) const;

  /**
   * Set airspace database to use
   * @param _airspace_database Pointer to the airspace database or NULL
   */
  void SetAirspaces(const Airspaces *_airspace_database) {
    airspace_database = _airspace_database;
  }

  /**
   * Set RasterTerrain to use
   * @param _terrain Pointer to the RasterTerrain or NULL
   */
  void SetTerrain(const RasterTerrain *_terrain) {
    terrain = _terrain;
  }

  /**
   * Set CrossSection range
   * @param range Range to draw [m]
   */
  void SetRange(double range) {
    vec.distance = range;
  }

  /**
   * Set CrossSection direction
   * @param bearing Direction to draw
   */
  void SetDirection(Angle bearing) {
    vec.bearing = bearing;
  }

  /**
   * Set CrossSection start point
   * @param _start Start GeoPoint to use for drawing
   */
  void SetStart(GeoPoint _start) {
    start = _start;
  }

  void SetInvalid() {
    vec.SetInvalid();
    start.SetInvalid();
  }

protected:
  void UpdateTerrain(TerrainHeight *elevations) const;

  void PaintGlide(ChartRenderer &chart) const;
  void PaintAircraft(Canvas &canvas, const ChartRenderer &chart,
                     const PixelRect rc) const;
  void PaintGrid(Canvas &canvas, ChartRenderer &chart) const;
  void PaintWorking(ChartRenderer &chart) const;
};

#endif
