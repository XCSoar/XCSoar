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

#include "HeightMatrix.hpp"
#include "RasterMap.hpp"
#include "Projection.hpp"

#include <assert.h>

void
HeightMatrix::SetSize(size_t _size)
{
  assert(_size > 0);

  data.grow_discard(_size);
}

void
HeightMatrix::SetSize(unsigned _width, unsigned height)
{
  width = _width;
  SetSize(width * height);
}

void
HeightMatrix::SetSize(unsigned width, unsigned height,
                      unsigned quantisation_pixels)
{
  SetSize((width + quantisation_pixels - 1) / quantisation_pixels,
          (height + quantisation_pixels - 1) / quantisation_pixels);
}

void
HeightMatrix::Fill(const RasterMap &map, const Projection &projection,
                   RECT rc, unsigned quantisation_pixels)
{
  SetSize((rc.right - rc.left + quantisation_pixels - 1) / quantisation_pixels,
          (rc.bottom - rc.top + quantisation_pixels - 1) / quantisation_pixels);

#ifndef SLOW_TERRAIN_STUFF
  // This code is quickest (by a little) but not so readable

  const GEOPOINT PanLocation = projection.GetPanLocation();
  const fixed InvDrawScale = projection.GetScreenScaleToLonLat();
  const POINT Orig_Screen = projection.GetOrigScreen();
  const int cost = projection.GetDisplayAngle().ifastcosine();
  const int sint = projection.GetDisplayAngle().ifastsine();

  for (int y = rc.top; y < rc.bottom; y += quantisation_pixels) {
    const int dy = y - Orig_Screen.y;
    const int dycost = dy * cost+512;
    const int dysint = dy * sint-512;

    unsigned short *p = data.begin() + (y * width + rc.left) / quantisation_pixels;

    for (int x = rc.left; x < rc.right; x += quantisation_pixels) {
      const int dx = x - Orig_Screen.x;
      const POINT r = { (dx * cost - dysint) / 1024,
                        (dycost + dx * sint) / 1024 };

      GEOPOINT gp;
      gp.Latitude = PanLocation.Latitude - Angle::native(r.y * InvDrawScale);
      gp.Longitude = PanLocation.Longitude + Angle::native(r.x * InvDrawScale)
        * gp.Latitude.invfastcosine();

      *p++ = max((short)0, map.GetField(gp));
    }

    assert(p <= data.end());
  }

#else

  // This code is marginally slower but readable

  for (int y = rc.top; y < rc.bottom; y += quantisation_pixels) {
    unsigned short *p = data + (y * width + rc.left) / quantisation_pixels;
    for (int x = rc.left; x < rc.right; x += quantisation_pixels) {
      GEOPOINT gp;
      projection.Screen2LonLat(x, y, gp);
      *p++ = max((short)0, map.GetField(gp));
    }

    assert(p <= data + size);
  }

#endif
}
