// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ObservationZoneClient.hpp"
#include "ObservationZonePoint.hpp"
#include "Boundary.hpp"
#include "Task/Points/TaskPoint.hpp"

ObservationZoneClient::~ObservationZoneClient() noexcept = default;

bool
ObservationZoneClient::IsInSector(const GeoPoint &location) const noexcept
{
  return oz_point->IsInSector(location);
}

bool
ObservationZoneClient::CanStartThroughTop() const noexcept
{
  return oz_point->CanStartThroughTop();
}

GeoPoint
ObservationZoneClient::GetRandomPointInSector(const double mag) const noexcept
{
  return oz_point->GetRandomPointInSector(mag);
}

double
ObservationZoneClient::ScoreAdjustment() const noexcept
{
  return oz_point->ScoreAdjustment();
}

OZBoundary
ObservationZoneClient::GetBoundary() const noexcept
{
  return oz_point->GetBoundary();
}

bool
ObservationZoneClient::TransitionConstraint(const GeoPoint &location,
                                            const GeoPoint &last_location) const noexcept
{
  return oz_point->TransitionConstraint(location, last_location);
}

void
ObservationZoneClient::SetLegs(const TaskPoint *previous,
                               const TaskPoint *next) noexcept
{
  oz_point->SetLegs(previous != nullptr ? &previous->GetLocation() : nullptr,
                    next != nullptr ? &next->GetLocation() : nullptr);
}
