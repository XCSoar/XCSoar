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

#include "Projection.hpp"

#include "Math/Earth.hpp"
#include "Math/Angle.hpp"
#include "Screen/Layout.hpp"

Projection::Projection() :
  GeoLocation(Angle::native(fixed_zero), Angle::native(fixed_zero)),
  ScreenRotation(Angle::native(fixed_zero)),
  scale(fixed_zero)
{
}

GeoPoint
Projection::ScreenToGeo(int x, int y) const
{
  const FastIntegerRotation::Pair p =
    ScreenRotation.Rotate(x - ScreenOrigin.x, y - ScreenOrigin.y);

  GeoPoint g(Angle::radians(fixed(p.first) * InvDrawScale),
             Angle::radians(fixed(p.second) * InvDrawScale));

  g.Latitude = GeoLocation.Latitude - g.Latitude;
  g.Longitude = GeoLocation.Longitude + g.Longitude * g.Latitude.invfastcosine();

  return g;
}

POINT
Projection::GeoToScreen(const GeoPoint &g) const
{
  const GeoPoint d = GeoLocation-g;
  const FastIntegerRotation::Pair p =
    ScreenRotation.Rotate((int)(d.Longitude.value_radians()
                              * g.Latitude.fastcosine() * DrawScale),
                        (int)(d.Latitude.value_radians() * DrawScale));

  POINT sc;
  sc.x = ScreenOrigin.x - p.first;
  sc.y = ScreenOrigin.y + p.second;
  return sc;
}

void
Projection::GeoToScreen(const GeoPoint *ptin, POINT *ptout,
                          unsigned n) const
{
  const GeoPoint *p = ptin;
  const GeoPoint *ptend = ptin + n;

  while (p < ptend) {
    *ptout++ = GeoToScreen(*p);
    p++;
  }
}

void 
Projection::SetScale(const fixed _scale)
{
  scale = _scale;

  // Calculate earth radius in pixels
  DrawScale = fixed_earth_r * scale;
  // Save inverted value for faster calculations
  InvDrawScale = fixed_one / DrawScale;
}

fixed
Projection::GetMapScaleUser() const
{
  return fixed(GetMapResolutionFactor()) / scale;
}

int
Projection::GetMapResolutionFactor()
{
  return IBLSCALE(30);
}
