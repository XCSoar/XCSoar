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

#include "GrahamScan.hpp"
#include "Geo/SearchPointVector.hpp"

#include <algorithm>

static constexpr int
Sign(double value, double tolerance) noexcept
{
  if (value > tolerance)
    return 1;
  if (value < -tolerance)
    return -1;

  return 0;
}

[[gnu::pure]]
static int
Direction(const GeoPoint &p0, const GeoPoint &p1, const GeoPoint &p2,
          double tolerance) noexcept
{
  //
  // In this program we frequently want to look at three consecutive
  // points, p0, p1, and p2, and determine whether p2 has taken a turn
  // to the left or a turn to the right.
  //
  // We can do this by by translating the points so that p1 is at the origin,
  // then taking the cross product of p0 and p2. The result will be positive,
  // negative, or 0, meaning respectively that p2 has turned right, left, or
  // is on a straight line.
  //

  const auto delta_a = p0 - p1;
  const auto delta_b = p2 - p1;

  const auto a = delta_a.longitude.Native() * delta_b.latitude.Native();
  const auto b = delta_b.longitude.Native() * delta_a.latitude.Native();

  if (tolerance < 0)
    /* auto-tolerance - this has been verified by experiment */
    tolerance = std::max(fabs(a), fabs(b)) / 10;

  return Sign(a - b, tolerance);
}

[[gnu::pure]]
static auto
Sorted(std::vector<SearchPoint> v) noexcept
{
  std::sort(v.begin(), v.end(), [](const SearchPoint &sp1, const SearchPoint &sp2){
    const auto &gp1 = sp1.GetLocation();
    const auto &gp2 = sp2.GetLocation();
    if (gp1.longitude < gp2.longitude)
      return true;
    else if (gp1.longitude == gp2.longitude)
      return gp1.latitude < gp2.latitude;
    else
      return false;
  });

  return v;
}

struct GrahamPartitions {
  SearchPoint left, right;
  std::vector<SearchPoint> upper;
  std::vector<SearchPoint> lower;
  bool pruned = false;
};

[[gnu::pure]]
static GrahamPartitions
PartitionPoints(const std::vector<SearchPoint> &src, double tolerance) noexcept
{
  GrahamPartitions result;
  result.upper.reserve(src.size());
  result.lower.reserve(src.size());

  //
  // The initial array of points is stored in vector raw_points. I first
  // sort it, which gives me the far left and far right points of the
  // hull.  These are special values, and they are stored off separately
  // in the left and right members.
  //
  // I then go through the list of raw_points, and one by one determine
  // whether each point is above or below the line formed by the right
  // and left points.  If it is above, the point is moved into the
  // upper_partition_points sequence. If it is below, the point is moved
  // into the lower_partition_points sequence. So the output of this
  // routine is the left and right points, and the sorted points that
  // are in the upper and lower partitions.
  //

  //
  // Step one in partitioning the points is to sort the raw data
  //
  const auto raw_points = Sorted(src);

  //
  // The the far left and far right points, remove them from the
  // sorted sequence and store them in special members
  //

  result.left = raw_points.front();
  result.right = raw_points.back();

  const auto begin = std::next(raw_points.cbegin());
  const auto end = std::prev(raw_points.cend());

  //
  // Now put the remaining points in one of the two output sequences
  //

  GeoPoint loclast = result.left.GetLocation();

  for (auto j = begin; j != end; ++j) {
    const auto &i = *j;

    if (loclast.longitude != i.GetLocation().longitude ||
        loclast.latitude != i.GetLocation().latitude) {
      loclast = i.GetLocation();

      int dir = Direction(result.left.GetLocation(),
                          result.right.GetLocation(),
                          i.GetLocation(), tolerance);
      if (dir < 0)
        result.upper.push_back(i);
      else
        result.lower.push_back(i);
    } else {
      result.pruned = true;
    }
  };

  return result;
}

static bool
BuildHalfHull(const SearchPoint &left, const SearchPoint &right,
              std::vector<SearchPoint> &&input,
              std::vector<SearchPoint> &output,
              double tolerance, int factor) noexcept
{
  //
  // This is the method that builds either the upper or the lower half convex
  // hull. It takes as its input a copy of the input array, which will be the
  // sorted list of points in one of the two halfs. It produces as output a list
  // of the points in the corresponding convex hull.
  //
  // The factor should be 1 for the lower hull, and -1 for the upper hull.
  //

  output.reserve(input.size() + 1);

  //
  // The hull will always start with the left point, and end with the
  // right point. According, we start by adding the left point as the
  // first point in the output sequence, and make sure the right point
  // is the last point in the input sequence.
  //
  input.push_back(right);
  output.push_back(left);

  bool pruned = false;

  //
  // The construction loop runs until the input is exhausted
  //
  for (const auto &i : input) {
    //
    // Repeatedly add the leftmost point to the hull, then test to see
    // if a convexity violation has occured. If it has, fix things up
    // by removing the next-to-last point in the output sequence until
    // convexity is restored.
    //

    /* remove all trailing points which would violate convexity with
       the point to be added */
    while (output.size() >= 2) {
      const auto last = std::prev(output.end());

      if (factor * Direction(std::prev(last)->GetLocation(),
                             i.GetLocation(),
                             last->GetLocation(),
                             tolerance) > 0)
        break;

      output.pop_back();
      pruned = true;
    }

    output.push_back(i);
  }

  return pruned;
}

struct GrahamHull {
  std::vector<SearchPoint> lower;
  std::vector<SearchPoint> upper;
  bool pruned;
};

[[gnu::pure]]
static GrahamHull
BuildHull(GrahamPartitions &&partitions, double tolerance) noexcept
{
  //
  // Building the hull consists of two procedures: building the lower
  // and then the upper hull. The two procedures are nearly identical -
  // the main difference between the two is the test for convexity. When
  // building the upper hull, our rull is that the middle point must
  // always be *above* the line formed by its two closest
  // neighbors. When building the lower hull, the rule is that point
  // must be *below* its two closest neighbors. We pass this information
  // to the building routine as the last parameter, which is either -1
  // or 1.
  //

  GrahamHull hull;

  bool lower_pruned = BuildHalfHull(partitions.left, partitions.right,
                                    std::move(partitions.lower),
                                    hull.lower, tolerance, 1);
  bool upper_pruned = BuildHalfHull(partitions.left, partitions.right,
                                    std::move(partitions.upper),
                                    hull.upper, tolerance, -1);

  hull.pruned = partitions.pruned || lower_pruned || upper_pruned;

  return hull;
}

bool
PruneInterior(SearchPointVector &raw_vector, double tolerance) noexcept
{
  if (raw_vector.size() < 3) {
    return false;
    // nothing to do
  }

  const auto hull = BuildHull(PartitionPoints(raw_vector, tolerance),
                              tolerance);
  if (!hull.pruned)
    /* nothing was pruned */
    return false;

  SearchPointVector res;
  res.reserve(raw_vector.size());

  for (unsigned i = 0; i + 1 < hull.lower.size(); i++)
    res.push_back(hull.lower[i]);

  for (unsigned i = hull.upper.size() - 1; i >= 1; i--)
    res.push_back(hull.upper[i]);

  assert(res.size() <= raw_vector.size());
  raw_vector.swap(res);
  return true;
}
