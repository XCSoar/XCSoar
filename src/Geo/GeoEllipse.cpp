/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

GeoEllipse::GeoEllipse(const GeoPoint &f1, const GeoPoint &f2,
                       const GeoPoint &p, const FlatProjection &_projection)
  :projection(_projection),
   ell(projection.ProjectFloat(f1),
       projection.ProjectFloat(f2),
       projection.ProjectFloat(p))
{
}

GeoPoint 
GeoEllipse::Parametric(const double t) const
{
  const FlatPoint fp = ell.Parametric(t);
  return projection.Unproject(fp);
}

bool 
GeoEllipse::IntersectExtended(const GeoPoint &p, GeoPoint &i1,
                              GeoPoint &i2) const
{
  const FlatPoint pf = projection.ProjectFloat(p);
  FlatPoint i1f, i2f;
  if (ell.IntersectExtended(pf,i1f,i2f)) {
    i1 = projection.Unproject(i1f);
    i2 = projection.Unproject(i2f);
    return true;
  }

  return false;
}
