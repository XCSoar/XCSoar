/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "FlatTriangleFan.hpp"

void
FlatTriangleFan::calc_bb()
{
  assert(!vs.empty());

  VertexVector::const_iterator it = vs.begin(), end = vs.end();
  bb_self = FlatBoundingBox(*it);
  for (++it; it != end; ++it)
    bb_self.expand(*it);
}

void
FlatTriangleFan::add_point(const FlatGeoPoint &p)
{
  // avoid duplicates
  if (!vs.empty() && p == vs.back())
    return;

  vs.push_back(p);
}

bool
FlatTriangleFan::is_inside(const FlatGeoPoint &p) const
{
  if (!bb_self.is_inside(p))
    return false;

  bool inside = false;
  for (VertexVector::const_iterator i = vs.begin(), j = vs.end() - 1;
       i != vs.end(); j = i++) {
    if ((i->Latitude > p.Latitude) == (j->Latitude > p.Latitude))
      continue;

    if ((p.Longitude < (j->Longitude - i->Longitude) *
                       (p.Latitude - i->Latitude) /
                       (j->Latitude - i->Latitude) + i->Longitude))
      inside = !inside;
  }

  return inside;
}
