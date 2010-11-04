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

#ifndef XCSOAR_WINDOW_PROJECTION_HPP
#define XCSOAR_WINDOW_PROJECTION_HPP

#include "Projection.hpp"
#include "Geo/GeoBounds.hpp"

class WindowProjection:
  public Projection
{
public:
  /**
   * Converts a geographical location to a screen coordinate if the
   * location is within the visible bounds
   * @param loc Geographical location
   * @param sc Screen coordinate (output)
   * @return True if the location is within the bounds
   */
  bool GeoToScreenIfVisible(const GeoPoint &loc, POINT &sc) const;

  /**
   * Checks whether a geographical location is within the visible bounds
   * @param loc Geographical location
   * @return True if the location is within the bounds
   */
  gcc_pure
  bool GeoVisible(const GeoPoint &loc) const;

  /**
   * Checks whether a screen coordinate is within the visible bounds
   * @param P Screen coordinate
   * @return True if the screen coordinate is within the bounds
   */
  gcc_pure
  bool ScreenVisible(const POINT &P) const;

  const RECT &GetMapRect() const {
    return MapRect;
  }

  /**
   * Returns the width of the map area in pixels.
   */
  unsigned GetScreenWidth() const {
    return MapRect.right - MapRect.left;
  }

  /**
   * Returns the height of the map area in pixels.
   */
  unsigned GetScreenHeight() const {
    return MapRect.bottom - MapRect.top;
  }

  gcc_pure
  fixed DistancePixelsToMeters(const int x) const {
    return x * GetMapScale() / GetMapResolutionFactor();
  }

  /**
   * Returns the width of the map area in meters.
   */
  fixed GetScreenWidthMeters() const {
    return DistancePixelsToMeters(GetScreenWidth());
  }

  gcc_pure
  fixed GetScreenDistanceMeters() const;

  // used by terrain renderer, topology and airspace
  gcc_pure
  GeoBounds CalculateScreenBounds(const fixed scale) const;

protected:
  RECT MapRect;

  gcc_pure
  static long max_dimension(const RECT &rc);

  void UpdateScreenBounds();

private:
  GeoBounds screenbounds_latlon;
};

#endif
