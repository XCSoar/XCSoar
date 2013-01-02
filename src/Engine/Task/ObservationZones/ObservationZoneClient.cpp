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

#include "ObservationZoneClient.hpp"
#include "ObservationZonePoint.hpp"
#include "Boundary.hpp"
#include "Task/Points/TaskPoint.hpp"

ObservationZoneClient::~ObservationZoneClient() {
  delete oz_point;
}

bool
ObservationZoneClient::IsInSector(const GeoPoint &location) const
{
  return oz_point->IsInSector(location);
}

bool
ObservationZoneClient::CanStartThroughTop() const
{
  return oz_point->CanStartThroughTop();
}

GeoPoint
ObservationZoneClient::GetRandomPointInSector(const fixed mag) const
{
  return oz_point->GetRandomPointInSector(mag);
}

fixed
ObservationZoneClient::ScoreAdjustment() const
{
  return oz_point->ScoreAdjustment();
}

GeoPoint
ObservationZoneClient::GetBoundaryParametric(fixed t) const
{
  return oz_point->GetBoundaryParametric(t);
}

OZBoundary
ObservationZoneClient::GetBoundary() const
{
  return oz_point->GetBoundary();
}

bool
ObservationZoneClient::TransitionConstraint(const GeoPoint &location,
                                            const GeoPoint &last_location) const
{
  return oz_point->TransitionConstraint(location, last_location);
}

void 
ObservationZoneClient::SetLegs(const TaskPoint *previous,
                               const TaskPoint *current,
                               const TaskPoint *next)
{
  oz_point->SetLegs(previous != NULL ? &previous->GetLocation() : NULL,
                    current != NULL ? &current->GetLocation() : NULL,
                    next != NULL ? &next->GetLocation() : NULL);
}
