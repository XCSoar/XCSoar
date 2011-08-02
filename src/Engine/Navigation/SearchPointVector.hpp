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
#ifndef SEARCHPOINTVECTOR_HPP
#define SEARCHPOINTVECTOR_HPP

#include "SearchPoint.hpp"
#include <vector>
class FlatRay;
class FlatBoundingBox;

typedef std::vector<SearchPoint> SearchPointVector;

bool prune_interior(SearchPointVector& spv);
bool is_convex(const SearchPointVector& spv);

/**
 * Apply convex pruning algorithm with increasing tolerance
 * until the trace is smaller than the given size
 *
 * @return True if input was modified
 */
bool
thin_to_size(SearchPointVector& spv, const unsigned max_size);

void project(SearchPointVector& spv, const TaskProjection& tp);

FlatGeoPoint nearest_point(const SearchPointVector& spv, 
                           const FlatGeoPoint &p);

/**
 * Find iterator of nearest point, assuming polygon is convex
 */
SearchPointVector::const_iterator
nearest_index_convex(const SearchPointVector& spv,
                     const FlatGeoPoint &p);

bool intersects(const SearchPointVector& spv,
                const FlatRay& ray);

FlatBoundingBox
compute_boundingbox(const SearchPointVector& spv);

/**
 * increment iterator, wrapping around to start if required
 */
void
circular_next(SearchPointVector::const_iterator &i, const SearchPointVector& spv);

/**
 * decreement iterator, wrapping around to last item if required
 */
void
circular_previous(SearchPointVector::const_iterator &i, const SearchPointVector& spv);

#endif
