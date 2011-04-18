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

#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"

void
ProtectedAirspaceWarningManager::visit_warnings(AirspaceWarningVisitor& visitor) const
{
  Lease lease(*this);
  lease->visit_warnings(visitor);
}

void
ProtectedAirspaceWarningManager::clear()
{
  ExclusiveLease lease(*this);
  lease->clear();
}

void
ProtectedAirspaceWarningManager::reset(const AIRCRAFT_STATE &as)
{
  ExclusiveLease lease(*this);
  lease->reset(as);
}

void
ProtectedAirspaceWarningManager::clear_warnings()
{
  ExclusiveLease lease(*this);
  lease->acknowledge_all();
}

size_t
ProtectedAirspaceWarningManager::warning_size() const
{
  Lease lease(*this);
  return lease->size();
}

bool
ProtectedAirspaceWarningManager::warning_empty() const
{
  Lease lease(*this);
  return lease->empty();
}

int
ProtectedAirspaceWarningManager::get_warning_index(const AbstractAirspace& airspace) const
{
  Lease lease(*this);
  return lease->get_warning_index(airspace);
}

void
ProtectedAirspaceWarningManager::acknowledge_day(const AbstractAirspace& airspace,
                                                 const bool set)
{
  ExclusiveLease lease(*this);
  lease->acknowledge_day(airspace, set);
}

void
ProtectedAirspaceWarningManager::acknowledge_warning(const AbstractAirspace& airspace,
                                                     const bool set)
{
  ExclusiveLease lease(*this);
  lease->acknowledge_warning(airspace, set);
}

void
ProtectedAirspaceWarningManager::acknowledge_inside(const AbstractAirspace& airspace,
                                                    const bool set)
{
  ExclusiveLease lease(*this);
  lease->acknowledge_inside(airspace, set);
}

void
ProtectedAirspaceWarningManager::reset_warning(const AIRCRAFT_STATE &as)
{
  ExclusiveLease lease(*this);
  lease->reset(as);
}

bool
ProtectedAirspaceWarningManager::update_warning(const AIRCRAFT_STATE &as,
                                                const bool circling,
                                                const unsigned dt)
{
  ExclusiveLease lease(*this);
  return lease->update(as, circling, dt);
}
