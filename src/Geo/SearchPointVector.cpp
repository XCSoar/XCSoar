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

#include "SearchPointVector.hpp"
#include "GeoBounds.hpp"
#include "ConvexHull/GrahamScan.hpp"
#include "ConvexHull/PolygonInterior.hpp"
#include "Flat/FlatRay.hpp"
#include "Flat/FlatBoundingBox.hpp"

bool 
SearchPointVector::PruneInterior()
{
  GrahamScan gs(*this);
  return gs.PruneInterior();
}

bool
SearchPointVector::ThinToSize(const unsigned max_size)
{
  static constexpr double tolerance = 1.0e-8;
  unsigned i = 2;
  bool retval = false;
  while (size() > max_size) {
    GrahamScan gs(*this, tolerance * i);
    retval |= gs.PruneInterior();
    i *= i;
  }
  return retval;
}

void 
SearchPointVector::Project(const FlatProjection &tp)
{
  for (auto &i : *this)
    i.Project(tp);
}

gcc_pure
static FlatGeoPoint
NearestPoint(const FlatGeoPoint &p1, const FlatGeoPoint &p2,
              const FlatGeoPoint &p3)
{
  const FlatGeoPoint p12 = p2-p1;
  const double rsq(p12.DotProduct(p12));
  if (rsq <= 0)
    return p1;

  const FlatGeoPoint p13 = p3-p1;
  const double numerator(p13.DotProduct(p12));
  
  if (numerator <= 0) {
    return p1;
  } else if (numerator>= rsq) {
    return p2;
  } else {
    double t = numerator/rsq;
    return p1+(p2-p1)*t;
  }
}

gcc_pure
static FlatGeoPoint
SegmentNearestPoint(const SearchPointVector& spv,
                      const SearchPointVector::const_iterator i1,
                      const FlatGeoPoint &p3)
{
  if (i1+1 == spv.end()) {
    return NearestPoint(i1->GetFlatLocation(),
                        spv.begin()->GetFlatLocation(),
                        p3);
  } else {
    return NearestPoint(i1->GetFlatLocation(),
                        (i1 + 1)->GetFlatLocation(),
                        p3);
  }
}

gcc_pure
static FlatGeoPoint
NearestPointNonConvex(const SearchPointVector& spv, const FlatGeoPoint &p3)
{
  unsigned distance_min = 0-1;
  FlatGeoPoint point_best;

  for (auto i = spv.begin(); i!= spv.end(); ++i) {

    FlatGeoPoint pa = SegmentNearestPoint(spv,i,p3);
    unsigned d_this = p3.DistanceSquared(pa);
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
  for (auto i = begin(); i != end(); ++i) {
    unsigned d_this = p3.DistanceSquared(i->GetFlatLocation());
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
    return (*this)[0].GetFlatLocation();

  return NearestPointNonConvex(*this, p3);
}

bool
SearchPointVector::IntersectsWith(const FlatRay &ray) const
{
  for (auto it = begin(); it + 1 != end(); ++it) {
    const FlatRay r_seg(it->GetFlatLocation(), (it + 1)->GetFlatLocation());

    if (r_seg.IntersectsDistinct(ray))
      return true;
  }
  return false;
}

FlatBoundingBox
SearchPointVector::CalculateBoundingbox() const
{
  if (empty())
    return FlatBoundingBox(FlatGeoPoint(0,0),FlatGeoPoint(0,0));

  FlatBoundingBox bb((*this)[0].GetFlatLocation());
  for (const auto &i : *this)
    bb.Expand(i.GetFlatLocation());
  bb.ExpandByOne(); // add 1 to fix rounding
  return bb;
}

GeoBounds
SearchPointVector::CalculateGeoBounds() const
{
  GeoBounds bb = GeoBounds::Invalid();
  for (const auto &i : *this)
    bb.Extend(i.GetLocation());

  return bb;
}

SearchPointVector::const_iterator
SearchPointVector::NextCircular(const_iterator i) const
{
  i++;
  if (i == end())
    i = begin();
  return i;
}

SearchPointVector::const_iterator
SearchPointVector::PreviousCircular(const_iterator i) const
{
  if (i == begin())
    i = begin() + size() - 1;
  else
    i--;
  return i;
}

bool
SearchPointVector::IsInside(const GeoPoint &pt) const
{
  return PolygonInterior(pt, begin(), end());
}

bool
SearchPointVector::IsInside(const FlatGeoPoint &pt) const
{
  return PolygonInterior(pt, begin(), end());
}
