/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_PROJECTION_HPP
#define XCSOAR_PROJECTION_HPP

#include "Navigation/GeoPoint.hpp"
#include "Math/FastRotation.hpp"
#include "Compiler.h"

#include <windef.h>

class Projection {
public:
  Projection();

  virtual fixed GetMapScaleUser() const;

  /**
   * Converts screen coordinates to a GeoPoint
   * @param x x-Coordinate on the screen
   * @param y y-Coordinate on the screen
   */
  gcc_pure
  GeoPoint ScreenToGeo(int x, int y) const;

  /**
   * Converts a GeoPoint to screen coordinates
   * @param g GeoPoint to convert
   */
  gcc_pure
  POINT GeoToScreen(const GeoPoint &location) const;

  /**
   * Converts a LatLon-based polygon to screen coordinates
   * @param ptin Input polygon
   * @param ptout Output polygon
   * @param n Number of points in the polygon
   */
  void GeoToScreen(const GeoPoint *ptin, POINT *ptout,
                     unsigned n) const;

  /**
   * Returns the origin/rotation center in screen coordinates
   * @return The origin/rotation center in screen coordinates
   */
  const POINT &GetScreenOrigin() const {
    return ScreenOrigin;
  }
  /**
   * Set the origin/rotation center to the given screen coordinates
   * @param x Screen coordinate in x-direction
   * @param y Screen coordinate in y-direction
   */
  void SetScreenOrigin(int x, int y) {
    ScreenOrigin.x = x;
    ScreenOrigin.y = y;
  }
  /**
   * Set the origin/rotation center to the given screen coordinates
   * @param pt Screen coordinate
   */
  void SetScreenOrigin(POINT pt) {
    ScreenOrigin = pt;
  }

  /**
   * Returns the GeoPoint at the ScreenOrigin
   * @return GeoPoint at the ScreenOrigin
   */
  const GeoPoint &GetGeoLocation() const {
    return GeoLocation;
  }
  /**
   * Set the GeoPoint that relates to the ScreenOrigin
   * @param g The new GeoPoint
   */
  void SetGeoLocation(GeoPoint g) {
    GeoLocation = g;
  }

  fixed GetScreenToGeoScale() const {
    return InvDrawScale;
  }

  fixed GetGeoToScreenScale() const {
    return DrawScale;
  }

  unsigned DistanceMetersToScreen(const fixed x) const {
    return iround(m_scale_meters_to_screen * x);
  }

  Angle GetScreenAngle() const {
    return ScreenRotation.GetAngle();
  }
  void SetScreenAngle(Angle angle) {
    ScreenRotation.SetAngle(angle);
  }

protected:
  gcc_const
  static int GetMapResolutionFactor();

  void SetScale(const fixed scale_meters_to_screen);

private:
  /** This is the geographical location that the ScreenOrigin is mapped to */
  GeoPoint GeoLocation;

  /**
   * This is the point that the ScreenRotation will rotate around.
   * It is also the point that the GeoLocation points to.
   */
  POINT ScreenOrigin;

  FastIntegerRotation ScreenRotation;

  fixed DrawScale;
  fixed InvDrawScale;
  fixed m_scale_meters_to_screen;
};

#endif
