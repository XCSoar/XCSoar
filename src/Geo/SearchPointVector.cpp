// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SearchPointVector.hpp"
#include "GeoBounds.hpp"
#include "ConvexHull/GrahamScan.hpp"
#include "ConvexHull/PolygonInterior.hpp"
#include "Flat/FlatRay.hpp"
#include "Flat/FlatBoundingBox.hpp"

#include <limits.h> // for UINT_MAX

bool
SearchPointVector::PruneInterior() noexcept
{
  return ::PruneInterior(*this);
}

bool
SearchPointVector::ThinToSize(const unsigned max_size) noexcept
{
  static constexpr double tolerance = 1.0e-8;
  double i = 2;
  bool retval = false;
  while (size() > max_size) {
    retval |= ::PruneInterior(*this, tolerance * i);
    i *= i;
  }
  return retval;
}

void
SearchPointVector::Project(const FlatProjection &tp) noexcept
{
  for (auto &i : *this)
    i.Project(tp);
}

[[gnu::pure]]
static FlatGeoPoint
NearestPoint(const FlatGeoPoint &p1, const FlatGeoPoint &p2,
             const FlatGeoPoint &p3) noexcept
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

[[gnu::pure]]
static FlatGeoPoint
SegmentNearestPoint(const SearchPointVector& spv,
                    const SearchPointVector::const_iterator i1,
                    const FlatGeoPoint &p3) noexcept
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

[[gnu::pure]]
static FlatGeoPoint
NearestPointNonConvex(const SearchPointVector &spv,
                      const FlatGeoPoint &p3) noexcept
{
  unsigned distance_min = UINT_MAX;
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
SearchPointVector::NearestIndexConvex(const FlatGeoPoint &p3) const noexcept
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
SearchPointVector::NearestPoint(const FlatGeoPoint &p3) const noexcept
{
  // special case
  if (empty())
    return p3; // really should be error

  if (size() == 1)
    return (*this)[0].GetFlatLocation();

  return NearestPointNonConvex(*this, p3);
}

bool
SearchPointVector::IntersectsWith(const FlatRay &ray) const noexcept
{
  for (auto it = begin(); it + 1 != end(); ++it) {
    const FlatRay r_seg(it->GetFlatLocation(), (it + 1)->GetFlatLocation());

    if (r_seg.IntersectsDistinct(ray))
      return true;
  }
  return false;
}

FlatBoundingBox
SearchPointVector::CalculateBoundingbox() const noexcept
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
SearchPointVector::CalculateGeoBounds() const noexcept
{
  GeoBounds bb = GeoBounds::Invalid();
  for (const auto &i : *this)
    bb.Extend(i.GetLocation());

  return bb;
}

SearchPointVector::const_iterator
SearchPointVector::NextCircular(const_iterator i) const noexcept
{
  ++i;
  if (i == end())
    i = begin();
  return i;
}

SearchPointVector::const_iterator
SearchPointVector::PreviousCircular(const_iterator i) const noexcept
{
  if (i == begin())
    i = begin() + size() - 1;
  else
    --i;
  return i;
}

bool
SearchPointVector::IsInside(const GeoPoint &pt) const noexcept
{
  return PolygonInterior(pt, begin(), end());
}

bool
SearchPointVector::IsInside(const FlatGeoPoint &pt) const noexcept
{
  return PolygonInterior(pt, begin(), end());
}
