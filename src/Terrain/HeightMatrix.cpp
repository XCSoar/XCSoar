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
#include "WindowProjection.hpp"

#include <algorithm>
#include <assert.h>

void
HeightMatrix::SetSize(size_t _size)
{
  assert(_size > 0);

  data.grow_discard(_size);
}

void
HeightMatrix::SetSize(unsigned _width, unsigned _height)
{
  width = _width;
  height = _height;

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
HeightMatrix::Fill(const RasterMap &map, const WindowProjection &projection,
                   unsigned quantisation_pixels)
{
  const unsigned screen_width = projection.GetScreenWidth();
  const unsigned screen_height = projection.GetScreenHeight();

  SetSize((screen_width + quantisation_pixels - 1) / quantisation_pixels,
          (screen_height + quantisation_pixels - 1) / quantisation_pixels);

  minimum = 0x7fff;
  maximum = 0;

#ifndef SLOW_TERRAIN_STUFF
  // This code is quickest (by a little) but not so readable

  const GeoPoint PanLocation = projection.GetGeoLocation();
  const fixed InvDrawScale = projection.GetScreenToGeoScale();
  const POINT Orig_Screen = projection.GetScreenOrigin();
  const int cost = projection.GetScreenAngle().ifastcosine();
  const int sint = projection.GetScreenAngle().ifastsine();
#endif

  for (unsigned y = 0; y < screen_height; y += quantisation_pixels) {
#ifndef SLOW_TERRAIN_STUFF
    const int dy = y - Orig_Screen.y;
    const int dycost = dy * cost+512;
    const int dysint = dy * sint-512;
#endif

    short *p = data.begin() + y * width / quantisation_pixels;

    for (unsigned x = 0; x < screen_width; x += quantisation_pixels) {
#ifndef SLOW_TERRAIN_STUFF
      const int dx = x - Orig_Screen.x;
      const POINT r = { (dx * cost - dysint) / 1024,
                        (dycost + dx * sint) / 1024 };

      GeoPoint gp;
      gp.Latitude = PanLocation.Latitude - Angle::native(r.y * InvDrawScale);
      gp.Longitude = PanLocation.Longitude + Angle::native(r.x * InvDrawScale)
        * gp.Latitude.invfastcosine();
#else
      GeoPoint gp = projection.ScreenToGeo(x, y);
#endif

      short h = map.GetField(gp);
      if (h != RasterBuffer::TERRAIN_INVALID) {
        if (h < minimum)
          minimum = h;
        if (h > maximum)
          maximum = h;
      }

      *p++ = h;
    }

    assert(p <= data.end());
  }
}
