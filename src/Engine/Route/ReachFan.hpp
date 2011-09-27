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

#ifndef REACHFAN_HPP
#define REACHFAN_HPP

#include "Navigation/TaskProjection.hpp"
#include "FlatTriangleFanTree.hpp"
#include "Rough/RoughAltitude.hpp"

class RoutePolars;
class RasterMap;
struct GeoBounds;

class ReachFan {
  TaskProjection task_proj;
  FlatTriangleFanTree root;
  RoughAltitude terrain_base;

public:
  ReachFan():terrain_base(0) {};

  friend class PrintHelper;

  void reset();

  bool solve(const AGeoPoint origin,
             const RoutePolars &rpolars,
             const RasterMap *terrain,
             const bool do_solve=true);

  bool find_positive_arrival(const AGeoPoint dest,
                             const RoutePolars &rpolars,
                             RoughAltitude &arrival_height_reach,
                             RoughAltitude &arrival_height_direct) const;

  bool is_inside(const GeoPoint origin, const bool turning=true) const;

  void accept_in_range(const GeoBounds& bounds,
                       TriangleFanVisitor& visitor) const;

  RoughAltitude get_terrain_base() const {
    return terrain_base;
  }
};

#endif
