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
#include "GeoEllipse.hpp"

GeoEllipse::GeoEllipse(const GEOPOINT &f1, const GEOPOINT &f2,
                       const GEOPOINT &p,
                       const TaskProjection &_task_projection): 
  task_projection(_task_projection)
{
  ell = FlatEllipse(task_projection.fproject(f1),
                    task_projection.fproject(f2),
                    task_projection.fproject(p));
}

GEOPOINT 
GeoEllipse::parametric(const fixed t) const {
  const FlatPoint fp = ell.parametric(t);
  return task_projection.funproject(fp);
}

bool 
GeoEllipse::intersect_extended(const GEOPOINT &p,
                               GEOPOINT &i1,
                               GEOPOINT &i2) const 
{
  const FlatPoint pf = task_projection.fproject(p);
  FlatPoint i1f, i2f;
  if (ell.intersect_extended(pf,i1f,i2f)) {
    i1 = task_projection.funproject(i1f);
    i2 = task_projection.funproject(i2f);
    return true;
  } else {
    return false;
  }
}

