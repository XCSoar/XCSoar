/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"

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
TaskLeg::GetOrigin() const
{
  return destination.GetPrevious();
}

const OrderedTaskPoint *
TaskLeg::GetNext() const
{
  return destination.GetNext();
}

OrderedTaskPoint *
TaskLeg::GetNext()
{
  return destination.GetNext();
}

GeoVector 
TaskLeg::GetPlannedVector() const
{
  if (!GetOrigin()) {
    return GeoVector(fixed_zero);
  } else {
    return memo_planned.calc(GetOrigin()->GetLocationRemaining(),
                             destination.GetLocationRemaining());
  }
}

GeoVector 
TaskLeg::GetRemainingVector(const GeoPoint &ref) const
{
  switch (destination.GetActiveState()) {
  case OrderedTaskPoint::AFTER_ACTIVE:
    if (!GetOrigin())
      return GeoVector(fixed_zero);

    // this leg totally included
    return memo_remaining.calc(GetOrigin()->GetLocationRemaining(),
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
TaskLeg::GetTravelledVector(const GeoPoint &ref) const
{
  switch (destination.GetActiveState()) {
  case OrderedTaskPoint::BEFORE_ACTIVE:
    if (!GetOrigin())
      return GeoVector(fixed_zero);

    // this leg totally included
    return memo_travelled.calc(GetOrigin()->GetLocationTravelled(),
                               destination.GetLocationTravelled());

  case OrderedTaskPoint::CURRENT_ACTIVE:
    // this leg partially included
    if (!GetOrigin())
      return GeoVector(fixed_zero, 
                       ref.Bearing(destination.GetLocationRemaining()));

    if (destination.HasEntered())
      return memo_travelled.calc(GetOrigin()->GetLocationTravelled(),
                                 destination.GetLocationTravelled());
    else
      return memo_travelled.calc(GetOrigin()->GetLocationTravelled(), ref);

  case OrderedTaskPoint::AFTER_ACTIVE:
    if (!GetOrigin())
      return GeoVector(fixed_zero);

    // this leg may be partially included
    if (GetOrigin()->HasEntered())
      return memo_travelled.calc(GetOrigin()->GetLocationTravelled(), ref);

  default:
    return GeoVector(fixed_zero);
  };
}

fixed 
TaskLeg::GetScoredDistance(const GeoPoint &ref) const
{
  if (!GetOrigin())
    return fixed_zero;

  switch (destination.GetActiveState()) {
  case OrderedTaskPoint::BEFORE_ACTIVE:
    // this leg totally included
    return 
      max(fixed_zero,
          GetOrigin()->GetLocationScored().Distance(destination.GetLocationScored())
          - GetOrigin()->ScoreAdjustment()-destination.ScoreAdjustment());
    break;
  case OrderedTaskPoint::CURRENT_ACTIVE:
    // this leg partially included
    if (destination.HasEntered()) {
      max(fixed_zero,
          GetOrigin()->GetLocationScored().Distance(destination.GetLocationScored())
          - GetOrigin()->ScoreAdjustment()-destination.ScoreAdjustment());
    } else {
      return 
        max(fixed_zero,
            ref.ProjectedDistance(GetOrigin()->GetLocationScored(),
                                  destination.GetLocationScored())
                 -GetOrigin()->ScoreAdjustment());
    }
    break;
  case OrderedTaskPoint::AFTER_ACTIVE:
    // this leg may be partially included
    if (GetOrigin()->HasEntered()) {
      return max(fixed_zero,
                 memo_travelled.calc(GetOrigin()->GetLocationScored(),
                                     ref).distance
                      -GetOrigin()->ScoreAdjustment());
    }
  default:
    return fixed_zero;
    break;
  };
  return fixed_zero;
}

fixed 
TaskLeg::GetNominalLegDistance() const
{
  return GetNominalLegVector().distance;
}

GeoVector
TaskLeg::GetNominalLegVector() const
{
  if (!GetOrigin()) {
    return GeoVector(fixed_zero);
  } else {
    return memo_nominal.calc(GetOrigin()->GetLocation(),
                             destination.GetLocation());
  }
}

fixed 
TaskLeg::GetMaximumLegDistance() const
{
  if (GetOrigin())
    return memo_max.Distance(GetOrigin()->GetLocationMax(),
                             destination.GetLocationMax());
  return fixed_zero;
}

fixed 
TaskLeg::GetMinimumLegDistance() const
{
  if (GetOrigin())
    return memo_min.Distance(GetOrigin()->GetLocationMin(),
                             destination.GetLocationMin());
  return fixed_zero;
}

fixed 
TaskLeg::ScanDistanceTravelled(const GeoPoint &ref)
{
  vector_travelled = GetTravelledVector(ref);
  return vector_travelled.distance +
    (GetNext() ? GetNext()->ScanDistanceTravelled(ref) : fixed_zero);
}

fixed 
TaskLeg::ScanDistanceRemaining(const GeoPoint &ref)
{
  vector_remaining = GetRemainingVector(ref);
  return vector_remaining.distance +
    (GetNext() ? GetNext()->ScanDistanceRemaining(ref) : fixed_zero);
}

fixed 
TaskLeg::ScanDistancePlanned()
{
  vector_planned = GetPlannedVector();
  return vector_planned.distance +
    (GetNext() ? GetNext()->ScanDistancePlanned() : fixed_zero);
}

fixed 
TaskLeg::ScanDistanceMax() const
{
  return GetMaximumLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceMax() : fixed_zero);
}

fixed 
TaskLeg::ScanDistanceMin() const
{
  return GetMinimumLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceMin() : fixed_zero);
}

fixed 
TaskLeg::ScanDistanceNominal() const
{
  return GetNominalLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceNominal() : fixed_zero);
}

fixed 
TaskLeg::ScanDistanceScored(const GeoPoint &ref) const
{
  return GetScoredDistance(ref) +
    (GetNext() ? GetNext()->ScanDistanceScored(ref) : fixed_zero);
}
