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

#include "GrahamScan.hpp"
#include "Navigation/SearchPointVector.hpp"

/*
static bool
operator==(const SearchPoint& s1, const SearchPoint& s2)
{
  return s1.equals(s2);
}
*/

static bool
sortleft (const SearchPoint& sp1, const SearchPoint& sp2)
{ 
  return sp1.sort(sp2);
}

GrahamScan::GrahamScan(SearchPointVector& sps, const fixed sign_tolerance):
  raw_points(sps.begin(), sps.end()), raw_vector(sps), size(sps.size()),
  tolerance(sign_tolerance)
{
}

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
void GrahamScan::partition_points()
{
  //
  // Step one in partitioning the points is to sort the raw data
  //
  raw_points.sort(sortleft);

  //
  // The the far left and far right points, remove them from the
  // sorted sequence and store them in special members
  //

  left = &raw_points.front();
  right = &raw_points.back();

  //
  // Now put the remaining points in one of the two output sequences
  //

  GeoPoint loclast = left->get_location();

  upper_partition_points.reserve(size);
  lower_partition_points.reserve(size);

  for (std::list<SearchPoint>::iterator i = raw_points.begin(); 
       i != raw_points.end(); ) {

    if ((loclast.longitude != (*i).get_location().longitude)
        ||(loclast.latitude != (*i).get_location().latitude)) {
      loclast = (*i).get_location();

      int dir = direction( left->get_location(), right->get_location(), 
                           (*i).get_location(), tolerance );
      SearchPoint* sp = &(*i);
      if ( dir < 0 )
        upper_partition_points.push_back( sp );
      else
        lower_partition_points.push_back( sp );
    }
    ++i;
  };

}

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
void GrahamScan::build_hull()
{
  build_half_hull(lower_partition_points, lower_hull, 1 );
  build_half_hull(upper_partition_points, upper_hull, -1 );
}

//
// This is the method that builds either the upper or the lower half convex
// hull. It takes as its input a copy of the input array, which will be the
// sorted list of points in one of the two halfs. It produces as output a list
// of the points in the corresponding convex hull.
//
// The factor should be 1 for the lower hull, and -1 for the upper hull.
//
void GrahamScan::build_half_hull( std::vector< SearchPoint* > input,
                                  std::vector< SearchPoint* > &output,
                                  int factor )
{
  output.reserve(input.size() + 1);

  //
  // The hull will always start with the left point, and end with the
  // right point. According, we start by adding the left point as the
  // first point in the output sequence, and make sure the right point
  // is the last point in the input sequence.
  //
  input.push_back( right );
  output.push_back( left );

  //
  // The construction loop runs until the input is exhausted
  //
  for (std::vector<SearchPoint *>::const_iterator i = input.begin();
       i != input.end(); ++i) {
    //
    // Repeatedly add the leftmost point to the hull, then test to see
    // if a convexity violation has occured. If it has, fix things up
    // by removing the next-to-last point in the output sequence until
    // convexity is restored.
    //
    output.push_back(*i);

    while ( output.size() >= 3 ) {
      size_t end = output.size() - 1;

      if ( factor * direction( output[ end - 2 ]->get_location(), 
                               output[ end ]->get_location(), 
                               output[ end - 1 ]->get_location(),
                               tolerance) <= 0 ) {
        output.erase( output.begin() + end - 1 );
      }
      else
        break;
    }
  }
}

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

int GrahamScan::direction( const GeoPoint& p0,
                           const GeoPoint& p1,
                           const GeoPoint& p2,
                           const fixed& tolerance)
{
  return (( (p0.longitude - p1.longitude ) * (p2.latitude - p1.latitude ) )
          - ( (p2.longitude - p1.longitude ) * (p0.latitude - p1.latitude ) )).Sign(tolerance);
}


bool 
GrahamScan::prune_interior()
{
  SearchPointVector res;
  res.reserve(size);

  if (size<3) {
    std::copy(raw_points.begin(), raw_points.end(),
              std::back_inserter(res));
    return false;
    // nothing to do
  }

  partition_points();
  build_hull();

  for ( unsigned i = 0 ; i+1 < lower_hull.size() ; i++ ) {
    res.push_back(*lower_hull[i]);
  }
  for ( int i = upper_hull.size()-1; i>=0 ; i-- ) {
    res.push_back(*upper_hull[i]);
  }

// if (!has_changed) {
//      *changed = !std::equal(res.begin(), res.end(), raw_vector.begin() );
//    }

  if (res.size() != size) {
    raw_vector.swap(res);
    return true;
  } else {
    return false;
  }
}
