/* Copyright_License {

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
#include "TaskProjection.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include <algorithm>

static const fixed fixed_scale(1000.0);

TaskProjection::TaskProjection()
{
  GEOPOINT zero;
  reset(zero);
}
  
void 
TaskProjection::reset(const GEOPOINT &ref) 
{
  location_min = ref;
  location_max = ref;
  location_mid = ref;
}


void TaskProjection::scan_location(const GEOPOINT &ref) 
{
  location_min.Longitude = min(ref.Longitude,
                               location_min.Longitude);
  location_max.Longitude = max(ref.Longitude,
                               location_max.Longitude);
  location_min.Latitude = min(ref.Latitude,
                              location_min.Latitude);
  location_max.Latitude = max(ref.Latitude,
                              location_max.Latitude);
}

bool
TaskProjection::update_fast()
{
  GEOPOINT old_loc = location_mid;
  fixed old_midloc = cos_midloc;

  location_mid.Longitude = (location_max.Longitude+location_min.Longitude)*fixed_half;
  location_mid.Latitude = (location_max.Latitude+location_min.Latitude)*fixed_half;
  cos_midloc = fastcosine(location_mid.Latitude)*fixed_scale;

  return (!(old_loc == location_mid)) || (cos_midloc != old_midloc);
}


FlatPoint
TaskProjection::fproject(const GEOPOINT& tp) const
{
  FlatPoint fp((tp.Longitude-location_mid.Longitude)*cos_midloc,
               (tp.Latitude-location_mid.Latitude)*fixed_scale);
  return fp;
}

GEOPOINT 
TaskProjection::funproject(const FlatPoint& fp) const
{
  GEOPOINT tp;
  tp.Longitude = fp.x/cos_midloc+location_mid.Longitude;
  tp.Latitude = fp.y/fixed_scale+location_mid.Latitude;
  return tp;
}

FLAT_GEOPOINT 
TaskProjection::project(const GEOPOINT& tp) const
{
  FlatPoint f = fproject(tp);
  FLAT_GEOPOINT fp;
  fp.Longitude = (int)(f.x+fixed_half);
  fp.Latitude = (int)(f.y+fixed_half);
  return fp;
}


GEOPOINT 
TaskProjection::unproject(const FLAT_GEOPOINT& fp) const
{
  GEOPOINT tp;
  tp.Longitude = fp.Longitude/cos_midloc+location_mid.Longitude;
  tp.Latitude = fp.Latitude/fixed_scale+location_mid.Latitude;
  return tp;
}


fixed
TaskProjection::fproject_range(const GEOPOINT &tp, const fixed range) const
{
  GEOPOINT fr;
  ::FindLatitudeLongitude(tp,fixed_zero,range,&fr);
  FlatPoint f = fproject(fr);
  FlatPoint p = fproject(tp);
  return fabs(f.y-p.y);
}

unsigned
TaskProjection::project_range(const GEOPOINT &tp, const fixed range) const
{
  return (int)(fproject_range(tp,range)+fixed_half);
}

