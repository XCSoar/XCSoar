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

#include "TaskLeg.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"

#include <assert.h>

inline const OrderedTaskPoint *
TaskLeg::GetOrigin() const
{
  return destination.GetPrevious();
}

inline const OrderedTaskPoint *
TaskLeg::GetNext() const
{
  return destination.GetNext();
}

inline OrderedTaskPoint *
TaskLeg::GetNext()
{
  return destination.GetNext();
}

inline GeoVector
TaskLeg::GetPlannedVector() const
{
  if (!GetOrigin()) {
    return GeoVector::Zero();
  } else {
    return memo_planned.calc(GetOrigin()->GetLocationRemaining(),
                             destination.GetLocationRemaining());
  }
}

inline GeoVector
TaskLeg::GetRemainingVector(const GeoPoint &ref) const
{
  switch (destination.GetActiveState()) {
  case OrderedTaskPoint::AFTER_ACTIVE:
    // this leg totally included
    return GetPlannedVector();

  case OrderedTaskPoint::CURRENT_ACTIVE: {
    // this leg partially included

    if (!ref.IsValid())
      /* if we don't have a GPS fix yet, we fall back to the "planned"
         vector unless this task leg has already been achieved */
      return destination.HasEntered()
        ? GeoVector::Zero()
        : GetPlannedVector();

    return memo_remaining.calc(ref, destination.GetLocationRemaining());
  }

  case OrderedTaskPoint::BEFORE_ACTIVE:
    // this leg not included
    return GeoVector::Zero();
  }

  gcc_unreachable();
  assert(false);
  return GeoVector::Invalid();
}

inline GeoVector
TaskLeg::GetTravelledVector(const GeoPoint &ref) const
{
  switch (destination.GetActiveState()) {
  case OrderedTaskPoint::BEFORE_ACTIVE:
    if (!GetOrigin())
      return GeoVector::Zero();

    // this leg totally included
    return memo_travelled.calc(GetOrigin()->GetLocationTravelled(),
                               destination.GetLocationTravelled());

  case OrderedTaskPoint::CURRENT_ACTIVE:
    // this leg partially included
    if (!GetOrigin())
      return GeoVector(0,
                       ref.IsValid()
                       ? ref.Bearing(destination.GetLocationRemaining())
                       : Angle::Zero());

    if (destination.HasEntered())
      return memo_travelled.calc(GetOrigin()->GetLocationTravelled(),
                                 destination.GetLocationTravelled());
    else if (!ref.IsValid())
      return GeoVector::Zero();
    else
      return memo_travelled.calc(GetOrigin()->GetLocationTravelled(), ref);

  case OrderedTaskPoint::AFTER_ACTIVE:
    if (!GetOrigin())
      return GeoVector::Zero();

    // this leg may be partially included
    if (GetOrigin()->HasEntered())
      return memo_travelled.calc(GetOrigin()->GetLocationTravelled(),
                                 ref.IsValid()
                                 ? ref
                                 : destination.GetLocationTravelled());

    return GeoVector::Zero();
  }

  gcc_unreachable();
  assert(false);
  return GeoVector::Invalid();
}

inline double
TaskLeg::GetScoredDistance(const GeoPoint &ref) const
{
  if (!GetOrigin())
    return 0;

  switch (destination.GetActiveState()) {
  case OrderedTaskPoint::BEFORE_ACTIVE:
    // this leg totally included
    return fdim(GetOrigin()->GetLocationScored().Distance(destination.GetLocationScored()),
                GetOrigin()->ScoreAdjustment()-destination.ScoreAdjustment());

  case OrderedTaskPoint::CURRENT_ACTIVE:
    // this leg partially included
    if (destination.HasEntered()) {
      return fdim(GetOrigin()->GetLocationScored().Distance(destination.GetLocationScored()),
                  GetOrigin()->ScoreAdjustment()-destination.ScoreAdjustment());
    } else if (ref.IsValid())
      return fdim(ref.ProjectedDistance(GetOrigin()->GetLocationScored(),
                                        destination.GetLocationScored()),
                  GetOrigin()->ScoreAdjustment());
    else
      return 0;

  case OrderedTaskPoint::AFTER_ACTIVE:
    // this leg may be partially included
    if (GetOrigin()->HasEntered() && ref.IsValid()) {
      return fdim(memo_travelled.calc(GetOrigin()->GetLocationScored(),
                                      ref).distance,
                  GetOrigin()->ScoreAdjustment());
    }

    return 0;
  }

  gcc_unreachable();
  assert(false);
  return 0;
}

GeoVector
TaskLeg::GetNominalLegVector() const
{
  if (!GetOrigin()) {
    return GeoVector::Zero();
  } else {
    return memo_nominal.calc(GetOrigin()->GetLocation(),
                             destination.GetLocation());
  }
}

inline double
TaskLeg::GetMaximumLegDistance() const
{
  if (GetOrigin())
    return memo_max.Distance(GetOrigin()->GetLocationMax(),
                             destination.GetLocationMax());
  return 0;
}

inline double
TaskLeg::GetMinimumLegDistance() const
{
  if (GetOrigin())
    return memo_min.Distance(GetOrigin()->GetLocationMin(),
                             destination.GetLocationMin());
  return 0;
}

double
TaskLeg::ScanDistanceTravelled(const GeoPoint &ref)
{
  vector_travelled = GetTravelledVector(ref);
  return vector_travelled.distance +
    (GetNext() ? GetNext()->ScanDistanceTravelled(ref) : 0);
}

double
TaskLeg::ScanDistanceRemaining(const GeoPoint &ref)
{
  vector_remaining = GetRemainingVector(ref);
  return vector_remaining.distance +
    (GetNext() ? GetNext()->ScanDistanceRemaining(ref) : 0);
}

double
TaskLeg::ScanDistancePlanned()
{
  vector_planned = GetPlannedVector();
  return vector_planned.distance +
    (GetNext() ? GetNext()->ScanDistancePlanned() : 0);
}

double
TaskLeg::ScanDistanceMax() const
{
  return GetMaximumLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceMax() : 0);
}

double
TaskLeg::ScanDistanceMin() const
{
  return GetMinimumLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceMin() : 0);
}

double
TaskLeg::ScanDistanceNominal() const
{
  return GetNominalLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceNominal() : 0);
}

double
TaskLeg::ScanDistanceScored(const GeoPoint &ref) const
{
  return GetScoredDistance(ref) +
    (GetNext() ? GetNext()->ScanDistanceScored(ref) : 0);
}
