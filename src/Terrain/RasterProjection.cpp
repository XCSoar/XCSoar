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

#include "RasterProjection.hpp"
#include "Geo/GeoBounds.hpp"
#include "Math/Earth.hpp"

void
RasterProjection::set(const GeoBounds &bounds,
                      unsigned width, unsigned height)
{
  x_scale = fixed(width) /
    (bounds.east - bounds.west).as_bearing().value_native();
  left = bounds.west.value_native() * x_scale;

  y_scale = fixed(height) /
    (bounds.north - bounds.south).as_bearing().value_native();
  top = bounds.north.value_native() * y_scale;
}

fixed
RasterProjection::pixel_distance(const GeoPoint &location, unsigned pixels) const
{
  enum {
    /**
     * This factor is used to reduce fixed point rounding errors.
     * x_scale and y_scale are quite large numbers, and building their
     * reciprocals may lose a lot of precision.
     */
    FACTOR = 4096,
  };

  GeoPoint p;

  Angle distance = Angle::native(fixed_sqrt_two * FACTOR / (x_scale / pixels));
  p = GeoPoint(location.Longitude + distance, location.Latitude);
  fixed x = Distance(location, p);

  distance = Angle::native(fixed_sqrt_two * FACTOR / (y_scale / pixels));
  p = GeoPoint(location.Longitude, location.Latitude + distance);
  fixed y = Distance(location, p);

  return max(x, y) / FACTOR;
}
