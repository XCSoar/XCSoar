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
#include "SearchPointVector.hpp"
#include "Navigation/ConvexHull/GrahamScan.hpp"
#include "Navigation/Flat/FlatRay.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"
#include "ConvexHull/PolygonInterior.hpp"
#include <algorithm>
#include <functional>

bool 
SearchPointVector::PruneInterior()
{
  GrahamScan gs(*this);
  return gs.prune_interior();
}

bool
SearchPointVector::ThinToSize(const unsigned max_size)
{
  const fixed tolerance = fixed(1.0e-8);
  unsigned i = 2;
  bool retval = false;
  while (size() > max_size) {
    GrahamScan gs(*this, tolerance * i);
    retval |= gs.prune_interior();
    i *= i;
  }
  return retval;
}

bool 
SearchPointVector::IsConvex() const
{
  SearchPointVector copy = *this;
  GrahamScan gs(copy);
  return !gs.prune_interior();
}

void 
SearchPointVector::Project(const TaskProjection& tp)
{
  for (iterator i = begin(); i != end(); ++i)
    i->project(tp);
}

static FlatGeoPoint
NearestPoint(const FlatGeoPoint &p1, const FlatGeoPoint &p2,
              const FlatGeoPoint &p3)
{
  const FlatGeoPoint p12 = p2-p1;
  const fixed rsq(p12.dot(p12));
  if (!positive(rsq)) {
    return p1;
  }
  const FlatGeoPoint p13 = p3-p1;
  const fixed numerator(p13.dot(p12));
  
  if (!positive(numerator)) {
    return p1;
  } else if (numerator>= rsq) {
    return p2;
  } else {
    fixed t = numerator/rsq;
    return p1+(p2-p1)*t;
  }
}

static FlatGeoPoint
SegmentNearestPoint(const SearchPointVector& spv,
                      const SearchPointVector::const_iterator i1,
                      const FlatGeoPoint &p3)
{
  if (i1+1 == spv.end()) {
    return NearestPoint(i1->get_flatLocation(),
                         spv.begin()->get_flatLocation(),
                         p3);
  } else {
    return NearestPoint(i1->get_flatLocation(),
                         (i1+1)->get_flatLocation(),
                         p3);
  }
}

static FlatGeoPoint
NearestPointNonConvex(const SearchPointVector& spv, const FlatGeoPoint &p3)
{
  unsigned distance_min = 0-1;
  FlatGeoPoint point_best;
  for (SearchPointVector::const_iterator i = spv.begin(); 
       i!= spv.end(); ++i) {

    FlatGeoPoint pa = SegmentNearestPoint(spv,i,p3);
    unsigned d_this = p3.distance_sq_to(pa);
    if (d_this<distance_min) {
      distance_min = d_this;
      point_best = pa;
    }
  }
  return point_best;
}

SearchPointVector::const_iterator
SearchPointVector::NearestIndexConvex(const FlatGeoPoint &p3) const
{
  unsigned distance_min = 0 - 1;

  const_iterator i_best = end();

  // find nearest point in vector
  for (const_iterator i = begin(); i != end(); ++i) {
    unsigned d_this = p3.distance_sq_to(i->get_flatLocation());
    if (d_this < distance_min) {
      distance_min = d_this;
      i_best = i;
    }
  }
  return i_best;
}

FlatGeoPoint
SearchPointVector::NearestPoint(const FlatGeoPoint &p3) const
{
  // special case
  if (empty())
    return p3; // really should be error

  if (size() == 1)
    return (*this)[0].get_flatLocation();

  return NearestPointNonConvex(*this, p3);
}

bool
SearchPointVector::IntersectsWith(const FlatRay &ray) const
{
  for (const_iterator it = begin(); it + 1 != end(); ++it) {
    const FlatRay r_seg(it->get_flatLocation(), (it + 1)->get_flatLocation());

    if (r_seg.intersects_distinct(ray))
      return true;
  }
  return false;
}

FlatBoundingBox
SearchPointVector::CalculateBoundingbox() const
{
  if (empty())
    return FlatBoundingBox(FlatGeoPoint(0,0),FlatGeoPoint(0,0));

  FlatBoundingBox bb((*this)[0].get_flatLocation());
  for (const_iterator v = begin(); v != end(); ++v)
    bb.expand(v->get_flatLocation());
  bb.expand(); // add 1 to fix rounding
  return bb;
}

GeoBounds
SearchPointVector::CalculateGeoBounds() const
{
  if (empty())
    return GeoBounds(GeoPoint(Angle::zero(), Angle::zero()));

  GeoBounds bb((*this)[0].get_location());
  for (const_iterator v = begin(); v != end(); ++v)
    bb.extend(v->get_location());

  return bb;
}

void
SearchPointVector::NextCircular(SearchPointVector::const_iterator &i) const
{
  i++;
  if (i == end())
    i = begin();
}

void
SearchPointVector::PreviousCircular(SearchPointVector::const_iterator &i) const
{
  if (i == begin())
    i = begin() + size() - 1;
  else
    i--;
}

bool
SearchPointVector::IsInside(const GeoPoint &pt) const
{
  return PolygonInterior(pt, *this);
}

bool
SearchPointVector::IsInside(const FlatGeoPoint &pt) const
{
  return PolygonInterior(pt, *this);
}
