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

  gcc_pure
  GeoPoint ScreenToGeo(int x, int y) const;

  gcc_pure
  POINT GeoToScreen(const GeoPoint &location) const;

  void GeoToScreen(const GeoPoint *ptin, POINT *ptout,
                     unsigned n) const;

  const POINT &GetScreenOrigin() const {
    return ScreenOrigin;
  }

  const GeoPoint &GetGeoLocation() const {
    return GeoLocation;
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

protected:
  gcc_const
  static int GetMapResolutionFactor();

  void SetScaleMetersToScreen(const fixed scale_meters_to_screen);

  GeoPoint GeoLocation;
  POINT ScreenOrigin;
  FastIntegerRotation ScreenRotation;

private:
  fixed DrawScale;
  fixed InvDrawScale;
  fixed m_scale_meters_to_screen;
};

#endif
