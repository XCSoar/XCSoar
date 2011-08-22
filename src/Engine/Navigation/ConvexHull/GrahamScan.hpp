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
#ifndef GRAHAM_SCAN_HPP
#define GRAHAM_SCAN_HPP

#include <list>

#include "Util/NonCopyable.hpp"
#include "Math/fixed.hpp"
#include "Compiler.h"

#include <vector>

class SearchPointVector;
class SearchPoint;
struct GeoPoint;

/**
 * Class used to build convex hulls from vector.  This ensures
 * the returned vector is closed, and may prune points.
 */
class GrahamScan: private NonCopyable
{
public :
/** 
 * Constructor.  Note that this class should be used temporarily only
 * 
 * @param sps Input vector of points (may be unordered)
 */
  GrahamScan(SearchPointVector& sps, const fixed sign_tolerance = fixed(1.0e-8));

/** 
 * Perform convex hull transformation
 * 
 * @return changed Return status as to whether input vector was altered (pruned) or not
 */
  bool prune_interior();

private :
  void partition_points();
  void build_hull();
  void build_half_hull( std::vector< SearchPoint* > input,
                        std::vector< SearchPoint* > &output,
                        int factor );

  gcc_pure
  static int direction(const GeoPoint& p0,
                       const GeoPoint& p1,
                       const GeoPoint& p2,
                       const fixed& _tolerance);

  std::list< SearchPoint > raw_points;
  SearchPoint *left;
  SearchPoint *right;
  std::vector< SearchPoint* > upper_partition_points;
  std::vector< SearchPoint* > lower_partition_points;
  std::vector< SearchPoint* > lower_hull;
  std::vector< SearchPoint* > upper_hull;
  SearchPointVector &raw_vector;
  const unsigned size;
  const fixed tolerance;
};


#endif
