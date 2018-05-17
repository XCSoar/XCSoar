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

#ifndef FLAT_TRIANGLE_FAN_TREE_HPP
#define FLAT_TRIANGLE_FAN_TREE_HPP

#include "Geo/Flat/FlatBoundingBox.hpp"
#include "Util/SliceAllocator.hpp"
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
  bool gaps_filled;

public:
  friend class PrintHelper;

  FlatTriangleFanTree(const unsigned char _depth = 0)
    :depth(_depth),
     gaps_filled(false) {}

  bool IsRoot() const {
    return depth == 0;
  }

  void Clear() {
    FlatTriangleFan::Clear();
    children.clear();
  }

  void CalcBB();

  gcc_pure
  bool IsInside(FlatGeoPoint p) const {
    return FlatTriangleFan::IsInside(p, IsRoot());
  }

  void FillReach(const AFlatGeoPoint &origin, ReachFanParms &parms);
  void DummyReach(const AFlatGeoPoint &origin);

  /**
   * Basic check for a state created by DummyReach().  If this method
   * returns true, then calls to FindPositiveArrival() are supposed to
   * be useless.
   */
  gcc_pure
  bool IsDummy() const {
    return vs.size() == 1 && children.empty();
  }

  /**
   * @return true if a valid fan has been filled, false to discard
   * this object
   */
  bool FillReach(const AFlatGeoPoint &origin,
                 const int index_low, const int index_high,
                 const ReachFanParms &parms);

  bool FillDepth(const AFlatGeoPoint &origin, ReachFanParms &parms);
  void FillGaps(const AFlatGeoPoint &origin, ReachFanParms &parms);

  bool CheckGap(const AFlatGeoPoint &n, const RouteLink &e_1,
                const RouteLink &e_2, ReachFanParms &parms);

  bool FindPositiveArrival(FlatGeoPoint n,
                           const ReachFanParms &parms,
                           int &arrival_height) const;

  void AcceptInRange(const FlatBoundingBox &bb,
                     FlatTriangleFanVisitor &visitor) const;

  void UpdateTerrainBase(FlatGeoPoint origin, ReachFanParms &parms);

  gcc_pure
  int DirectArrival(FlatGeoPoint dest, const ReachFanParms &parms) const;
};

#endif
