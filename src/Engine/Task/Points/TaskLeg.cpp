// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskLeg.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "util/Compiler.h"

#include <cassert>

inline const OrderedTaskPoint *
TaskLeg::GetOrigin() const noexcept
{
  return destination.GetPrevious();
}

inline const OrderedTaskPoint *
TaskLeg::GetNext() const noexcept
{
  return destination.GetNext();
}

inline OrderedTaskPoint *
TaskLeg::GetNext() noexcept
{
  return destination.GetNext();
}

inline GeoVector
TaskLeg::GetPlannedVector() const noexcept
{
  if (!GetOrigin()) {
    return GeoVector::Zero();
  } else {
    return memo_planned.calc(GetOrigin()->GetLocationRemaining(),
                             destination.GetLocationRemaining());
  }
}

inline GeoVector
TaskLeg::GetRemainingVector(const GeoPoint &ref) const noexcept
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
TaskLeg::GetTravelledVector(const GeoPoint &ref) const noexcept
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
TaskLeg::GetScoredDistance(const GeoPoint &ref) const noexcept
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
TaskLeg::GetNominalLegVector() const noexcept
{
  if (!GetOrigin()) {
    return GeoVector::Zero();
  } else {
    return memo_nominal.calc(GetOrigin()->GetLocation(),
                             destination.GetLocation());
  }
}

inline double
TaskLeg::GetMaximumTotalLegDistance() const noexcept
{
  if (GetOrigin())
    return memo_max_total.Distance(GetOrigin()->GetLocationMaxTotal(),
                                   destination.GetLocationMaxTotal());
  return 0;
}

inline double
TaskLeg::GetMaximumLegDistance() const noexcept
{
  if (GetOrigin())
    return memo_max.Distance(GetOrigin()->GetLocationMax(),
                             destination.GetLocationMax());
  return 0;
}

inline double
TaskLeg::GetMinimumLegDistance() const noexcept
{
  if (GetOrigin())
    return memo_min.Distance(GetOrigin()->GetLocationMin(),
                             destination.GetLocationMin());
  return 0;
}

double
TaskLeg::ScanDistanceTravelled(const GeoPoint &ref) noexcept
{
  vector_travelled = GetTravelledVector(ref);
  return vector_travelled.distance +
    (GetNext() ? GetNext()->ScanDistanceTravelled(ref) : 0);
}

double
TaskLeg::ScanDistanceRemaining(const GeoPoint &ref) noexcept
{
  vector_remaining = GetRemainingVector(ref);
  return vector_remaining.distance +
    (GetNext() ? GetNext()->ScanDistanceRemaining(ref) : 0);
}

double
TaskLeg::ScanDistancePlanned() noexcept
{
  vector_planned = GetPlannedVector();
  return vector_planned.distance +
    (GetNext() ? GetNext()->ScanDistancePlanned() : 0);
}

double
TaskLeg::ScanDistanceMaxTotal() const noexcept
{
  return GetMaximumTotalLegDistance() +
         (GetNext() ? GetNext()->ScanDistanceMaxTotal() : 0);
}

double
TaskLeg::ScanDistanceMax() const noexcept
{
  return GetMaximumLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceMax() : 0);
}

double
TaskLeg::ScanDistanceMin() const noexcept
{
  return GetMinimumLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceMin() : 0);
}

double
TaskLeg::ScanDistanceNominal() const noexcept
{
  return GetNominalLegDistance() +
    (GetNext() ? GetNext()->ScanDistanceNominal() : 0);
}

double
TaskLeg::ScanDistanceScored(const GeoPoint &ref) const noexcept
{
  return GetScoredDistance(ref) +
    (GetNext() ? GetNext()->ScanDistanceScored(ref) : 0);
}
