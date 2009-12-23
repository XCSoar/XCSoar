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
#include "SearchPointVector.hpp"
#include "Navigation/ConvexHull/GrahamScan.hpp"
#include <algorithm>
#include <functional>

bool 
prune_interior(SearchPointVector& spv)
{
  bool changed=false;
  GrahamScan gs(spv);
  spv = gs.prune_interior(&changed);
  return changed;
}

bool 
is_convex(const SearchPointVector& spv)
{
  bool changed=false;
  GrahamScan gs(spv);
  size_t size_before = spv.size();
  SearchPointVector res;
  res = gs.prune_interior(&changed);
  return res.size() != size_before;
}

void 
project(SearchPointVector& spv, const TaskProjection& tp)
{
  for (SearchPointVector::iterator i = spv.begin(); i!= spv.end(); ++i) {
    i->project(tp);
  }
}

FLAT_GEOPOINT nearest_point(const FLAT_GEOPOINT &p1,
                            const FLAT_GEOPOINT &p2,
                            const FLAT_GEOPOINT &p3)
{
  const FLAT_GEOPOINT p12 = p2-p1;
  const fixed rsq(p12.dot(p12));
  if (!positive(rsq)) {
    return p1;
  }
  const FLAT_GEOPOINT p13 = p3-p1;
  const fixed numerator(p13.dot(p12));
  
  if (!positive(numerator)) {
    return p1;
  } else if (numerator>= rsq) {
    return p2;
  } else {
    fixed t = numerator/rsq;
    return p1+(p2-p1)*t;
  }
}

FLAT_GEOPOINT segment_nearest_point(const SearchPointVector& spv, 
                                    const SearchPointVector::const_iterator i1,
                                    const FLAT_GEOPOINT &p3)
{
  if (i1+1 == spv.end()) {
    return nearest_point(i1->get_flatLocation(),
                         spv.begin()->get_flatLocation(),
                         p3);
  } else {
    return nearest_point(i1->get_flatLocation(),
                         (i1+1)->get_flatLocation(),
                         p3);
  }
}


FLAT_GEOPOINT nearest_point_nonconvex(const SearchPointVector& spv, 
                                      const FLAT_GEOPOINT &p3)
{
  unsigned distance_min = 0-1;
  SearchPointVector::const_iterator i_best = spv.end();
  for (SearchPointVector::const_iterator i = spv.begin(); 
       i!= spv.end(); ++i) {

    FLAT_GEOPOINT pa = segment_nearest_point(spv,i,p3);
    unsigned d_this = p3.distance_sq_to(pa);
    if (d_this<distance_min) {
      distance_min = d_this;
      i_best = i;
    }
  }
  return i_best->get_flatLocation();
}



FLAT_GEOPOINT nearest_point_convex(const SearchPointVector& spv, 
                                   const FLAT_GEOPOINT &p3)
{
  unsigned distance_min = 0-1;

  SearchPointVector::const_iterator i_best = spv.end();

  // find nearest point in vector
  for (SearchPointVector::const_iterator i = spv.begin(); 
       i!= spv.end(); ++i) {

    unsigned d_this = p3.distance_sq_to(i->get_flatLocation());
    if (d_this<distance_min) {
      distance_min = d_this;
      i_best = i;
    }
  }

  FLAT_GEOPOINT pc = i_best->get_flatLocation();

  // find nearest point on this segment
  FLAT_GEOPOINT pa = segment_nearest_point(spv,i_best,p3);
  if (!(pa == pc)) {
    unsigned d_seg = pa.distance_sq_to(p3);
    if (d_seg < distance_min) {
      distance_min = d_seg;
      pc = pa;
    }
  }

  // find nearest point on previous segment
  SearchPointVector::const_iterator i_prev;
  if (i_best == spv.begin()) {
    i_prev = spv.end()-1;
  } else {
    i_prev = i_best-1;
  }

  FLAT_GEOPOINT pb = segment_nearest_point(spv,i_prev,p3);
  if (!(pb == pc)) {
    unsigned d_seg = pb.distance_sq_to(p3);
    if (d_seg < distance_min) {
      distance_min = d_seg;
      pc = pb;
    }
  }

  return pc;
}

FLAT_GEOPOINT nearest_point(const SearchPointVector& spv, 
                            const FLAT_GEOPOINT &p3,
                            const bool is_convex)
{
  // special case
  if (spv.empty()) {
    return p3; // really should be error
  } else if (spv.size()==1) {
    return spv[0].get_flatLocation();
  }

  if (is_convex) {
    /** \todo Strictly speaking it isn't correct to use this function
     */
    return nearest_point_convex(spv,p3);
  } else {
    return nearest_point_nonconvex(spv,p3);
  }
}
