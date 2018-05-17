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

#include "Geo/GeoBounds.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

static GeoBounds
MakeGeoBounds(int west, int north, int east, int south)
{
  return GeoBounds(GeoPoint(Angle::Degrees(west), Angle::Degrees(north)),
                   GeoPoint(Angle::Degrees(east), Angle::Degrees(south)));
}

int main(int argc, char **argv)
{
  plan_tests(58);

  GeoPoint g(Angle::Degrees(2), Angle::Degrees(4));

  GeoBounds b(g);

  ok1(equals(b.GetEast(), 2));
  ok1(equals(b.GetWest(), 2));
  ok1(equals(b.GetNorth(), 4));
  ok1(equals(b.GetSouth(), 4));

  ok1(b.IsEmpty());

  g.latitude = Angle::Degrees(6);
  g.longitude = Angle::Degrees(8);
  b.Extend(g);

  ok1(equals(b.GetEast(), 8));
  ok1(equals(b.GetWest(), 2));
  ok1(equals(b.GetNorth(), 6));
  ok1(equals(b.GetSouth(), 4));

  ok1(!b.IsEmpty());

  g = b.GetCenter();
  ok1(equals(g.latitude, 5));
  ok1(equals(g.longitude, 5));

  ok1(b.IsInside(Angle::Degrees(7), Angle::Degrees(4.5)));
  ok1(!b.IsInside(Angle::Degrees(9), Angle::Degrees(4.5)));
  ok1(!b.IsInside(Angle::Degrees(7), Angle::Degrees(1)));
  ok1(!b.IsInside(Angle::Degrees(9), Angle::Degrees(1)));

  b = b.Scale(2);

  ok1(equals(b.GetEast(), 11));
  ok1(equals(b.GetWest(), -1));
  ok1(equals(b.GetNorth(), 7));
  ok1(equals(b.GetSouth(), 3));

  b = b.Scale(0.5);

  ok1(equals(b.GetEast(), 8));
  ok1(equals(b.GetWest(), 2));
  ok1(equals(b.GetNorth(), 6));
  ok1(equals(b.GetSouth(), 4));

  GeoBounds c = MakeGeoBounds(2, 6, 8, 4);
  const GeoBounds c2 = c;
  ok1(c.Overlaps(b));
  ok1(c.IntersectWith(b));
  ok1(equals(c.GetWest(), 2));
  ok1(equals(c.GetNorth(), 6));
  ok1(equals(c.GetEast(), 8));
  ok1(equals(c.GetSouth(), 4));
  ok1(b.IntersectWith(c2));
  ok1(equals(b.GetWest(), 2));
  ok1(equals(b.GetNorth(), 6));
  ok1(equals(b.GetEast(), 8));
  ok1(equals(b.GetSouth(), 4));

  GeoBounds d = MakeGeoBounds(2, 6, 7, 5);
  ok1(c.Overlaps(d));
  ok1(c.IntersectWith(d));
  ok1(equals(c.GetWest(), 2));
  ok1(equals(c.GetNorth(), 6));
  ok1(equals(c.GetEast(), 7));
  ok1(equals(c.GetSouth(), 5));
  ok1(d.IntersectWith(c2));
  ok1(equals(d.GetWest(), 2));
  ok1(equals(d.GetNorth(), 6));
  ok1(equals(d.GetEast(), 7));
  ok1(equals(d.GetSouth(), 5));

  d = MakeGeoBounds(8, 6, 1, 5);
  ok1(!c.Overlaps(d));
  ok1(!c.IntersectWith(d));

  const GeoBounds outer = MakeGeoBounds(10, 20, 20, 10);
  const GeoBounds inner = MakeGeoBounds(12, 18, 18, 12);

  GeoBounds x = outer;
  ok1(x.IntersectWith(inner));
  ok1(equals(x.GetWest(), inner.GetWest()));
  ok1(equals(x.GetNorth(), inner.GetNorth()));
  ok1(equals(x.GetEast(), inner.GetEast()));
  ok1(equals(x.GetSouth(), inner.GetSouth()));

  x = inner;
  ok1(x.IntersectWith(outer));
  ok1(equals(x.GetWest(), inner.GetWest()));
  ok1(equals(x.GetNorth(), inner.GetNorth()));
  ok1(equals(x.GetEast(), inner.GetEast()));
  ok1(equals(x.GetSouth(), inner.GetSouth()));

  return exit_status();
}
