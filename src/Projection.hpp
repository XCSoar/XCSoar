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
#include "Geo/BoundsRectangle.hpp"
#include "Math/FastRotation.hpp"
#include "Compiler.h"

#include <windef.h>

class Projection {
public:
  Projection();

  virtual fixed GetMapScaleUser() const;

  gcc_pure
  GeoPoint Screen2LonLat(int x, int y) const;

  gcc_pure
  POINT LonLat2Screen(const GeoPoint &location) const;

  void LonLat2Screen(const GeoPoint *ptin, POINT *ptout,
                     unsigned n, unsigned skip) const;

  const POINT &GetOrigScreen() const {
    return Orig_Screen;
  }

  const GeoPoint &GetPanLocation() const {
    return PanLocation;
  }

  bool LonLat2ScreenIfVisible(const GeoPoint &loc, POINT *sc) const;

  gcc_pure
  bool LonLatVisible(const GeoPoint &loc) const;

  gcc_pure
  bool PointVisible(const POINT &P) const;

  fixed GetScreenScaleToLonLat() const {
    return InvDrawScale;
  }

  fixed GetLonLatToScreenScale() const {
    return DrawScale;
  }

  unsigned DistanceMetersToScreen(const fixed x) const {
    return iround(m_scale_meters_to_screen * x);
  }

  Angle GetDisplayAngle() const {
    return DisplayAngle.GetAngle();
  }

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

  // used by terrain renderer, topology and airspace
  gcc_pure
  BoundsRectangle CalculateScreenBounds(const fixed scale) const;

protected:
  gcc_const
  static int GetMapResolutionFactor();

  void SetScaleMetersToScreen(const fixed scale_meters_to_screen);

  GeoPoint PanLocation;
  POINT Orig_Screen;
  FastIntegerRotation DisplayAngle;
  RECT MapRect;

  gcc_pure
  static long max_dimension(const RECT &rc);

  void UpdateScreenBounds();

private:
  fixed DrawScale;
  fixed InvDrawScale;
  fixed m_scale_meters_to_screen; 
  BoundsRectangle screenbounds_latlon;
};

#endif
