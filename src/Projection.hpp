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
#if !defined(XCSOAR_PROJECTION_HPP)
#define XCSOAR_PROJECTION_HPP

#include "Navigation/GeoPoint.hpp"
#include "Screen/shapelib/mapprimitive.h"
#include <windef.h>
#include "Math/FastMath.h"
#include "Math/FastRotation.hpp"

class Projection {
public:
  Projection();

  void    Screen2LonLat(const int &x, const int &y,
                        GEOPOINT &location) const;

  void    LonLat2Screen(const GEOPOINT &location,
                        POINT &sc) const;
  void    LonLat2Screen(const GEOPOINT *ptin, POINT *ptout,
                        unsigned n, unsigned skip) const;
  void    LonLat2Screen(const pointObj* const ptin, POINT *ptout,
                        const int n,
                        const int skip) const;

  POINT   GetOrigScreen(void) const { return Orig_Screen; }
  GEOPOINT GetPanLocation() const { return PanLocation; }

  bool LonLat2ScreenIfVisible(const GEOPOINT &loc,
			      POINT *sc) const;

  bool LonLatVisible(const GEOPOINT &loc) const;

  bool PointVisible(const POINT &P) const;

  fixed GetScreenScaleToLonLat() const {
    return InvDrawScale;
  }
  fixed GetLonLatToScreenScale() const {
    return DrawScale;
  }
  unsigned DistanceMetersToScreen(const fixed x) {
    return iround(m_scale_meters_to_screen*x);
  }
  fixed GetDisplayAngle() const {
    return DisplayAngle.GetAngle();
  }
  RECT GetMapRect() const {
    return MapRect;
  }

  // used by terrain renderer, topology and airspace
  rectObj CalculateScreenBounds(const fixed scale) const;

protected:
  void SetScaleMetersToScreen(const fixed scale_meters_to_screen);

  GEOPOINT PanLocation;
  POINT  Orig_Screen;
  FastIntegerRotation DisplayAngle;
  RECT   MapRect;

  long max_dimension(const RECT &rc) const;

  void UpdateScreenBounds();
private:
  fixed DrawScale;
  fixed InvDrawScale;
  fixed m_scale_meters_to_screen; 
  rectObj   screenbounds_latlon;
};

#endif

