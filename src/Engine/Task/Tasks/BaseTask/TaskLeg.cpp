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

#include "TaskLeg.hpp"

#include "OrderedTaskPoint.hpp"

#include <assert.h>
#include <algorithm>

TaskLeg::TaskLeg(OrderedTaskPoint &_destination):
  vector_travelled(fixed_zero),
  vector_remaining(fixed_zero),
  vector_planned(fixed_zero),
  destination(_destination)
{
}

const OrderedTaskPoint* 
TaskLeg::origin() const
{
  return destination.get_previous();
}

OrderedTaskPoint* 
TaskLeg::next() const
{
  return destination.get_next();
}

GeoVector 
TaskLeg::leg_vector_planned() const
{
  if (!origin()) {
    return GeoVector(fixed_zero);
  } else {
    return memo_planned.calc(origin()->GetLocationRemaining(), 
                             destination.GetLocationRemaining());
  }
}

GeoVector 
TaskLeg::leg_vector_remaining(const GeoPoint &ref) const
{
  switch (destination.getActiveState()) {
  case OrderedTaskPoint::AFTER_ACTIVE:
    if (!origin())
      return GeoVector(fixed_zero);

    // this leg totally included
    return memo_remaining.calc(origin()->GetLocationRemaining(), 
                               destination.GetLocationRemaining());
    break;
  case OrderedTaskPoint::CURRENT_ACTIVE:
    // this leg partially included
    return memo_remaining.calc(ref, destination.GetLocationRemaining());
    break;
  case OrderedTaskPoint::BEFORE_ACTIVE:
    // this leg not included
  default:
    assert(1); // error!
    return GeoVector(fixed_zero);
  };
}

GeoVector
TaskLeg::leg_vector_travelled(const GeoPoint &ref) const
{
  switch (destination.getActiveState()) {
  case OrderedTaskPoint::BEFORE_ACTIVE:
    if (!origin())
      return GeoVector(fixed_zero);

    // this leg totally included
    return memo_travelled.calc(origin()->GetLocationTravelled(), 
                               destination.GetLocationTravelled());

  case OrderedTaskPoint::CURRENT_ACTIVE:
    // this leg partially included
    if (!origin())
      return GeoVector(fixed_zero, 
                       ref.bearing(destination.GetLocationRemaining()));

    if (destination.HasEntered())
      return memo_travelled.calc(origin()->GetLocationTravelled(), 
                                 destination.GetLocationTravelled());
    else
      return memo_travelled.calc(origin()->GetLocationTravelled(), ref);

  case OrderedTaskPoint::AFTER_ACTIVE:
    if (!origin())
      return GeoVector(fixed_zero);

    // this leg may be partially included
    if (origin()->HasEntered())
      return memo_travelled.calc(origin()->GetLocationTravelled(), ref);

  default:
    return GeoVector(fixed_zero);
  };
}

fixed 
TaskLeg::leg_distance_scored(const GeoPoint &ref) const
{
  if (!origin())
    return fixed_zero;

  switch (destination.getActiveState()) {
  case OrderedTaskPoint::BEFORE_ACTIVE:
    // this leg totally included
    return 
      max(fixed_zero,
               origin()->GetLocationScored().distance(
                 destination.GetLocationScored())
               -origin()->ScoreAdjustment()-destination.ScoreAdjustment());
    break;
  case OrderedTaskPoint::CURRENT_ACTIVE:
    // this leg partially included
    if (destination.HasEntered()) {
      max(fixed_zero,
               origin()->GetLocationScored().distance( 
                 destination.GetLocationScored())
               -origin()->ScoreAdjustment()-destination.ScoreAdjustment());
    } else {
      return 
        max(fixed_zero,
                 ref.projected_distance(origin()->GetLocationScored(), 
                                        destination.GetLocationScored())
                 -origin()->ScoreAdjustment());
    }
    break;
  case OrderedTaskPoint::AFTER_ACTIVE:
    // this leg may be partially included
    if (origin()->HasEntered()) {
      return max(fixed_zero,
                      memo_travelled.calc(origin()->GetLocationScored(), 
                                          ref).Distance
                      -origin()->ScoreAdjustment());
    }
  default:
    return fixed_zero;
    break;
  };
  return fixed_zero;
}

fixed 
TaskLeg::leg_distance_nominal() const
{
  return leg_vector_nominal().Distance;
}

GeoVector
TaskLeg::leg_vector_nominal() const
{
  if (!origin()) {
    return GeoVector(fixed_zero);
  } else {
    return memo_nominal.calc(origin()->GetLocation(),
                             destination.GetLocation());
  }
}

fixed 
TaskLeg::leg_distance_max() const
{
  if (origin())
    return memo_max.Distance(origin()->GetLocationMax(), 
                             destination.GetLocationMax());
  return fixed_zero;
}

fixed 
TaskLeg::leg_distance_min() const
{
  if (origin())
    return memo_min.Distance(origin()->GetLocationMin(), 
                             destination.GetLocationMin());
  return fixed_zero;
}

fixed 
TaskLeg::scan_distance_travelled(const GeoPoint &ref) 
{
  vector_travelled = leg_vector_travelled(ref);
  return vector_travelled.Distance +
         (next() ? next()->scan_distance_travelled(ref) : fixed_zero);
}

fixed 
TaskLeg::scan_distance_remaining(const GeoPoint &ref) 
{
  vector_remaining = leg_vector_remaining(ref);
  return vector_remaining.Distance +
         (next() ? next()->scan_distance_remaining(ref) : fixed_zero);
}

fixed 
TaskLeg::scan_distance_planned() 
{
  vector_planned = leg_vector_planned();
  return vector_planned.Distance +
         (next() ? next()->scan_distance_planned() : fixed_zero);
}

fixed 
TaskLeg::scan_distance_max() const
{
  return leg_distance_max() +
         (next() ? next()->scan_distance_max() : fixed_zero);
}

fixed 
TaskLeg::scan_distance_min() const
{
  return leg_distance_min() +
         (next() ? next()->scan_distance_min() : fixed_zero);
}

fixed 
TaskLeg::scan_distance_nominal() const
{
  return leg_distance_nominal() +
         (next() ? next()->scan_distance_nominal() : fixed_zero);
}

fixed 
TaskLeg::scan_distance_scored(const GeoPoint &ref) const
{
  return leg_distance_scored(ref) +
         (next() ? next()->scan_distance_scored(ref) : fixed_zero);
}
