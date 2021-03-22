/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef FLAT_TRIANGLE_FAN_TREE_HPP
#define FLAT_TRIANGLE_FAN_TREE_HPP

#include "Geo/Flat/FlatBoundingBox.hpp"
#include "util/SliceAllocator.hxx"
#include "FlatTriangleFan.hpp"

#include <list>

class FlatProjection;
struct GeoPoint;
struct RouteLink;
struct AFlatGeoPoint;
struct ReachFanParms;
template<typename T> struct ConstBuffer;

class FlatTriangleFanVisitor {
public:
  virtual void VisitFan(FlatGeoPoint origin,
                        ConstBuffer<FlatGeoPoint> fan) = 0;
};

class FlatTriangleFanTree: public FlatTriangleFan
{
public:
  static constexpr unsigned REACH_MAX_FANS = 300;

private:
  typedef std::list<FlatTriangleFanTree,
                    GlobalSliceAllocator<FlatTriangleFanTree, 128u> > LeafVector;

  FlatBoundingBox bb_children;
  LeafVector children;
  const unsigned char depth;
  bool gaps_filled = false;

public:
  friend class PrintHelper;

  FlatTriangleFanTree(const unsigned char _depth = 0) noexcept
    :depth(_depth) {}

  bool IsRoot() const noexcept {
    return depth == 0;
  }

  void Clear() noexcept {
    FlatTriangleFan::Clear();
    children.clear();
  }

  void CalcBB() noexcept;

  [[gnu::pure]]
  bool IsInside(FlatGeoPoint p) const noexcept {
    return FlatTriangleFan::IsInside(p, IsRoot());
  }

  void FillReach(const AFlatGeoPoint &origin, ReachFanParms &parms) noexcept;
  void DummyReach(const AFlatGeoPoint &origin) noexcept;

  /**
   * Basic check for a state created by DummyReach().  If this method
   * returns true, then calls to FindPositiveArrival() are supposed to
   * be useless.
   */
  [[gnu::pure]]
  bool IsDummy() const noexcept {
    return vs.size() == 1 && children.empty();
  }

  /**
   * @return true if a valid fan has been filled, false to discard
   * this object
   */
  bool FillReach(const AFlatGeoPoint &origin,
                 const int index_low, const int index_high,
                 const ReachFanParms &parms) noexcept;

  bool FillDepth(const AFlatGeoPoint &origin, ReachFanParms &parms) noexcept;
  void FillGaps(const AFlatGeoPoint &origin, ReachFanParms &parms) noexcept;

  bool CheckGap(const AFlatGeoPoint &n, const RouteLink &e_1,
                const RouteLink &e_2, ReachFanParms &parms) noexcept;

  bool FindPositiveArrival(FlatGeoPoint n,
                           const ReachFanParms &parms,
                           int &arrival_height) const noexcept;

  void AcceptInRange(const FlatBoundingBox &bb,
                     FlatTriangleFanVisitor &visitor) const noexcept;

  void UpdateTerrainBase(FlatGeoPoint origin, ReachFanParms &parms) noexcept;

  [[gnu::pure]]
  int DirectArrival(FlatGeoPoint dest,
                    const ReachFanParms &parms) const noexcept;
};

#endif
