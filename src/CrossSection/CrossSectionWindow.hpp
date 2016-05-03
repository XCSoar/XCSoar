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

#ifndef CROSS_SECTION_WINDOW_HPP
#define CROSS_SECTION_WINDOW_HPP

#include "Screen/BufferWindow.hpp"
#include "CrossSectionRenderer.hpp"
#include "Look/InfoBoxLook.hpp"

struct CrossSectionLook;
struct AirspaceLook;
struct ChartLook;
struct MoreData;
struct DerivedInfo;
struct MapSettings;
class Airspaces;
class RasterTerrain;

/**
 * A Window which renders a terrain and airspace cross-section
 */
class CrossSectionWindow
  : public BufferWindow
{
protected:
  CrossSectionRenderer renderer;

public:
  /**
   * Constructor. Initializes most class members.
   */
  CrossSectionWindow(const CrossSectionLook &look,
                     const AirspaceLook &airspace_look,
                     const ChartLook &chart_look,
                     const InfoBoxLook &info_box_look):
      renderer(look, airspace_look, chart_look, info_box_look.inverse) {}

  void ReadBlackboard(const MoreData &basic,
                      const DerivedInfo &calculated,
                      const GlideSettings &glide_settings,
                      const GlidePolar &glide_polar,
                      const MapSettings &map_settings) {
    renderer.ReadBlackboard(basic, calculated,
                            glide_settings, glide_polar,
                            map_settings);
    Invalidate();
  }

  /**
   * Set airspace database to use
   * @param _airspace_database Pointer to the airspace database or NULL
   */
  void SetAirspaces(const Airspaces *airspace_database) {
    renderer.SetAirspaces(airspace_database);
  }

  /**
   * Set RasterTerrain to use
   * @param _terrain Pointer to the RasterTerrain or NULL
   */
  void SetTerrain(const RasterTerrain *terrain) {
    renderer.SetTerrain(terrain);
  }

  /**
   * Set CrossSection range
   * @param range Range to draw [m]
   */
  void SetRange(double range) {
    renderer.SetRange(range);
  }

  /**
   * Set CrossSection direction
   * @param bearing Direction to draw
   */
  void SetDirection(Angle bearing) {
    renderer.SetDirection(bearing);
  }

  /**
   * Set CrossSection start point
   * @param _start Start GeoPoint to use for drawing
   */
  void SetStart(GeoPoint start) {
    renderer.SetStart(start);
  }

  void SetInvalid() {
    renderer.SetInvalid();
  }

protected:
  /* virtual methods from AntiFlickerWindow */
  virtual void OnPaintBuffer(Canvas &canvas) override;
};

#endif
