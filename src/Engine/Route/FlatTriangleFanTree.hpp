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

#ifndef FLAT_TRIANGLE_FAN_TREE_HPP
#define FLAT_TRIANGLE_FAN_TREE_HPP

#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Util/SliceAllocator.hpp"
#include "FlatTriangleFan.hpp"

#include <list>

class TaskProjection;
struct RouteLink;
class AFlatGeoPoint;
struct ReachFanParms;

class TriangleFanVisitor
{
public:
  virtual void StartFan() = 0;
  virtual void AddPoint(const GeoPoint &p) = 0;
  virtual void EndFan() = 0;
};

class FlatTriangleFanTree: public FlatTriangleFan
{
public:
  static const unsigned REACH_MAX_FANS = 300;

  typedef std::list<FlatTriangleFanTree,
                    GlobalSliceAllocator<FlatTriangleFanTree, 128u> > LeafVector;

protected:
  FlatBoundingBox bb_children;
  LeafVector children;
  unsigned char depth;
  bool gaps_filled;

public:
  friend class PrintHelper;

  FlatTriangleFanTree(const unsigned char _depth = 0)
    :FlatTriangleFan(),
     bb_children(FlatGeoPoint(0,0)),
     depth(_depth),
     gaps_filled(false) {}

  void Clear() {
    FlatTriangleFan::clear();
    children.clear();
  }

  void CalcBB();

  gcc_pure
  bool IsInsideTree(const FlatGeoPoint &p,
                    const bool include_children = true) const;

  void FillReach(const AFlatGeoPoint &origin, ReachFanParms &parms);
  void DummyReach(const AFlatGeoPoint &origin);

  void FillReach(const AFlatGeoPoint &origin,
                 const int index_low, const int index_high,
                 ReachFanParms &parms);

  bool FillDepth(const AFlatGeoPoint &origin, ReachFanParms &parms);
  void FillGaps(const AFlatGeoPoint &origin, ReachFanParms &parms);

  bool CheckGap(const AFlatGeoPoint &n, const RouteLink &e_1,
                const RouteLink &e_2, ReachFanParms &parms);

  bool FindPositiveArrival(const FlatGeoPoint &n,
                           const ReachFanParms &parms,
                           RoughAltitude &arrival_height) const;

  void AcceptInRange(const FlatBoundingBox &bb,
                     const TaskProjection &task_proj,
                     TriangleFanVisitor &visitor) const;

  void UpdateTerrainBase(const FlatGeoPoint &origin, ReachFanParms &parms);

  gcc_pure
  RoughAltitude DirectArrival(const FlatGeoPoint &dest,
                              const ReachFanParms &parms) const;
};

#endif
