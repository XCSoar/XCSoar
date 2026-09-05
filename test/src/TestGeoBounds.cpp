// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Geo/GeoBounds.hpp"
#include "Geo/Quadrilateral.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

static GeoBounds
MakeGeoBounds(int west, int north, int east, int south)
{
  return GeoBounds(GeoPoint(Angle::Degrees(west), Angle::Degrees(north)),
                   GeoPoint(Angle::Degrees(east), Angle::Degrees(south)));
}

int main()
{
  plan_tests(74);

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

  /* GeoQuadrilateral::GetBounds must be wraparound-safe */
  const GeoQuadrilateral plain{
    GeoPoint(Angle::Degrees(10), Angle::Degrees(20)),
    GeoPoint(Angle::Degrees(30), Angle::Degrees(20)),
    GeoPoint(Angle::Degrees(10), Angle::Degrees(0)),
    GeoPoint(Angle::Degrees(30), Angle::Degrees(0)),
  };
  const GeoBounds plain_bounds = plain.GetBounds();
  ok1(equals(plain_bounds.GetWest(), 10));
  ok1(equals(plain_bounds.GetEast(), 30));
  ok1(equals(plain_bounds.GetNorth(), 20));
  ok1(equals(plain_bounds.GetSouth(), 0));
  ok1(plain_bounds.IsInside(GeoPoint(Angle::Degrees(20),
                                     Angle::Degrees(10))));

  /* corners on both sides of the antimeridian */
  const GeoQuadrilateral wrap{
    GeoPoint(Angle::Degrees(170), Angle::Degrees(10)),
    GeoPoint(Angle::Degrees(-170), Angle::Degrees(10)),
    GeoPoint(Angle::Degrees(170), Angle::Degrees(-10)),
    GeoPoint(Angle::Degrees(-170), Angle::Degrees(-10)),
  };
  const GeoBounds wrap_bounds = wrap.GetBounds();
  ok1(equals(wrap_bounds.GetWest(), 170));
  ok1(equals(wrap_bounds.GetEast(), -170));
  ok1(equals(wrap_bounds.GetNorth(), 10));
  ok1(equals(wrap_bounds.GetSouth(), -10));
  ok1(wrap_bounds.IsInside(GeoPoint(Angle::Degrees(180),
                                    Angle::Degrees(0))));
  ok1(wrap_bounds.IsInside(GeoPoint(Angle::Degrees(-180),
                                    Angle::Degrees(0))));
  ok1(!wrap_bounds.IsInside(GeoPoint(Angle::Degrees(0),
                                     Angle::Degrees(0))));

  /* Unnormalized longitudes past ±180° (as ScreenToGeo can produce) */
  const GeoQuadrilateral unnormalized{
    GeoPoint(Angle::Degrees(-188), Angle::Degrees(10)),
    GeoPoint(Angle::Degrees(-172), Angle::Degrees(10)),
    GeoPoint(Angle::Degrees(-188), Angle::Degrees(-10)),
    GeoPoint(Angle::Degrees(-172), Angle::Degrees(-10)),
  };
  const GeoBounds unnorm_bounds = unnormalized.GetBounds();
  ok1(equals(unnorm_bounds.GetWest(), 172));
  ok1(equals(unnorm_bounds.GetEast(), -172));
  ok1(unnorm_bounds.IsInside(GeoPoint(Angle::Degrees(180),
                                      Angle::Degrees(0))));
  ok1(unnorm_bounds.IsInside(GeoPoint(Angle::Degrees(175),
                                      Angle::Degrees(0))));

  return exit_status();
}
