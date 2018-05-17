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

#ifndef REACHFAN_HPP
#define REACHFAN_HPP

#include "Geo/Flat/FlatProjection.hpp"
#include "FlatTriangleFanTree.hpp"

class RoutePolars;
class RasterMap;
class GeoBounds;
struct ReachResult;

class ReachFan
{
  FlatProjection projection;
  FlatTriangleFanTree root;
  int terrain_base;

public:
  ReachFan():terrain_base(0) {}

  friend class PrintHelper;

  bool IsEmpty() const {
    return root.IsEmpty();
  }

  const FlatProjection &GetProjection() const {
    return projection;
  }

  void Reset();

  bool Solve(const AGeoPoint origin, const RoutePolars &rpolars,
             const RasterMap *terrain, const bool do_solve = true);

  bool FindPositiveArrival(const AGeoPoint dest, const RoutePolars &rpolars,
                           ReachResult &result_r) const;

  void AcceptInRange(const GeoBounds &bounds,
                     FlatTriangleFanVisitor &visitor) const;

  int GetTerrainBase() const {
    return terrain_base;
  }
};

#endif
