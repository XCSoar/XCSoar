/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Navigation/SearchPointVector.hpp"

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
  GrahamScan(const SearchPointVector& sps);

/** 
 * Perform convex hull transformation
 * 
 * @param changed If supplied, will return status as to whether input vector was altered (pruned) or not
 * 
 * @return Vector representing convex hull of input
 */
  SearchPointVector prune_interior(bool *changed=NULL);
private :
  void partition_points();
  void build_hull();
  void build_half_hull( std::vector< SearchPoint* > input,
                        std::vector< SearchPoint* > &output,
                        int factor );
  double direction( const GEOPOINT& p0,
                    const GEOPOINT& p1,
                    const GEOPOINT& p2 );
  std::list< SearchPoint > raw_points;
  SearchPoint *left;
  SearchPoint *right;
  std::vector< SearchPoint* > upper_partition_points;
  std::vector< SearchPoint* > lower_partition_points;
  std::vector< SearchPoint* > lower_hull;
  std::vector< SearchPoint* > upper_hull;
  const SearchPointVector &raw_vector;
};


#endif
