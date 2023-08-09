// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AATPoint.hpp"
#include "Geo/Flat/FlatProjection.hpp"
#include "Geo/Flat/FlatLine.hpp"
#include "util/Compiler.h"

const GeoPoint &
AATPoint::GetLocationRemaining() const noexcept
{
  if (!IsPast())
    return target_location;

  if (HasSampled())
    return GetLocationMax();

  return GetLocationMin();
}

bool
AATPoint::UpdateSampleNear(const AircraftState &state,
                           const FlatProjection &projection) noexcept
{
  bool retval = OrderedTaskPoint::UpdateSampleNear(state, projection);
  retval |= CheckTarget(state, false);

  return retval;
}

bool
AATPoint::UpdateSampleFar(const AircraftState &state,
                          const FlatProjection &projection) noexcept
{
  /* the orderedtaskpoint::update_sample_far does nothing for now but
     we are calling this in case that changes */
  return OrderedTaskPoint::UpdateSampleFar(state, projection) ||
    CheckTarget(state, true);
}

inline bool
AATPoint::CheckTarget(const AircraftState &state, const bool known_outside) noexcept
{
  if (IsCurrent() && target_locked)
    return false;

  bool moved = false;
  if (!known_outside && IsInSector(state))
    moved = CheckTargetInside(state);
  else
    moved = CheckTargetOutside(state);

  return moved;
}

bool
AATPoint::IsCloseToTarget(const AircraftState &state,
                          const double threshold) const noexcept
{
  if (!valid())
    return false;

  return DoubleLegDistance(state.location)
    - DoubleLegDistance(target_location) > -threshold;
}

inline bool
AATPoint::CheckTargetInside(const AircraftState &state) noexcept
{
  /* target must be moved if d(p_last,t)+d(t,p_next) <
     d(p_last,state)+d(state,p_next) */

  if (!IsCloseToTarget(state))
    return false;

  if (DoubleLegDistance(state.location) > DoubleLegDistance(GetLocationMax()))
    // no improvement available
    return false;

  target_location = state.location;
  return true;
}

inline bool
AATPoint::CheckTargetOutside([[maybe_unused]] const AircraftState &state) noexcept
{
  return false;
}

bool
AATPoint::SetRange(const double p, const bool force_if_current) noexcept
{
  if (target_locked)
    return false;

  switch (GetActiveState()) {
  case BEFORE_ACTIVE:
    return false;

  case CURRENT_ACTIVE:
    if (!HasEntered() || force_if_current) {
      target_location = InterpolateLocationMinMax(p);
      return true;
    }
    return false;

  case AFTER_ACTIVE:
    target_location = InterpolateLocationMinMax(p);
    return true;
  }

  assert(false);
  gcc_unreachable();
}

void
AATPoint::SetTarget(const GeoPoint &loc, const bool override_lock) noexcept
{
  if (override_lock || !target_locked)
    target_location = loc;
}

void
AATPoint::SetTarget(RangeAndRadial rar, const FlatProjection &proj) noexcept
{
  const auto fprev =
    proj.ProjectFloat(GetPrevious()->GetLocationRemaining());
  const auto floc = proj.ProjectFloat(GetLocation());
  const FlatLine flb (fprev,floc);
  const FlatLine fradius(floc,
                         proj.ProjectFloat(rar.range < 0
                                           ? GetLocationMin()
                                           : GetLocationMax()));
  const auto radius = fradius.GetDistance() * fabs(rar.range);

  const auto angle = rar.radial - flb.GetAngle();
  const auto [angle_sin, angle_cos] = angle.SinCos();
  const FlatPoint ftarget1{radius * angle_cos, radius * -angle_sin};

  const auto ftarget2 = floc + ftarget1;
  const auto targetG = proj.Unproject(ftarget2);

  SetTarget(targetG, true);
}

RangeAndRadial
AATPoint::GetTargetRangeRadial(double oldrange) const noexcept
{
  const auto fprev = GetPrevious()->GetLocationRemaining();
  const auto floc = GetLocation();
  const auto radialraw = (floc.Bearing(GetTargetLocation()) -
      fprev.Bearing(floc)).AsBearing();
  auto radial = radialraw.AsDelta();

  auto d = floc.Distance(GetTargetLocation());
  if (radial < -Angle::QuarterCircle() || radial > Angle::QuarterCircle())
    d = -d;

  const auto radius = d < 0
    ? floc.Distance(GetLocationMin())
    : floc.Distance(GetLocationMax());
  const auto range = std::clamp(d / radius, -1., 1.);

  if (oldrange == 0 && range == 0)
    radial = Angle::Zero();

  return RangeAndRadial{ range, radial };
}

bool
AATPoint::Equals(const OrderedTaskPoint &other) const noexcept
{
  const auto &tp = (const AATPoint &)other;

  return OrderedTaskPoint::Equals(other) &&
    target_locked == tp.target_locked &&
    target_location == tp.target_location;
}
