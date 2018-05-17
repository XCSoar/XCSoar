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

#ifndef GRAHAM_SCAN_HPP
#define GRAHAM_SCAN_HPP

#include "Util/NonCopyable.hpp"

#include <list>
#include <vector>

class SearchPointVector;
class SearchPoint;
struct GeoPoint;

/**
 * Class used to build convex hulls from vector.  This ensures
 * the returned vector is closed, and may prune points.
 *
 * @author http://www.drdobbs.com/cpp/201806315?pgno=4
 */
class GrahamScan: private NonCopyable
{
  std::list<SearchPoint> raw_points;
  SearchPoint *left;
  SearchPoint *right;
  std::vector<SearchPoint*> upper_partition_points;
  std::vector<SearchPoint*> lower_partition_points;
  std::vector<SearchPoint*> lower_hull;
  std::vector<SearchPoint*> upper_hull;
  SearchPointVector &raw_vector;
  const unsigned size;
  const double tolerance;

public:
  /**
   * Constructor.  Note that this class should be used temporarily only
   *
   * @param sps Input vector of points (may be unordered)
   *
   * @param sign_tolerance the tolerance for the direction sign; -1
   * for automatic tolerance
   */
  GrahamScan(SearchPointVector& sps, const double sign_tolerance = -1);

  /**
   * Perform convex hull transformation
   *
   * @return changed Return status as to whether input vector was altered (pruned) or not
   */
  bool PruneInterior();

private:
  void PartitionPoints();
  void BuildHull();
  void BuildHalfHull(std::vector<SearchPoint*> input,
                     std::vector<SearchPoint*> &output, int factor);
};


#endif
