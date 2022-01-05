/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_AIRSPACE_WARNING_COPY_HPP
#define XCSOAR_AIRSPACE_WARNING_COPY_HPP

#include "ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "util/StaticArray.hxx"
#include "Geo/GeoPoint.hpp"

class AirspaceWarningCopy
{
private:
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning, ids_acked;
  StaticArray<GeoPoint,32> locations;

  Serial serial;

public:
  auto GetSerial() const noexcept {
    return serial;
  }

  void Visit(const AirspaceWarning& as) noexcept {
    if (as.IsInside()) {
      ids_inside.checked_append(&as.GetAirspace());
    } else if (as.IsWarning()) {
      ids_warning.checked_append(&as.GetAirspace());
      locations.checked_append(as.GetSolution().location);
    }

    if (!as.IsAckExpired())
      ids_acked.checked_append(&as.GetAirspace());
  }

  void Visit(const AirspaceWarningManager &awm) noexcept {
    serial = awm.GetSerial();

    for (const auto &i : awm)
      Visit(i);
  }

  void Visit(const ProtectedAirspaceWarningManager &awm) noexcept {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Visit(lease);
  }

  const StaticArray<GeoPoint,32> &GetLocations() const noexcept {
    return locations;
  }

  bool HasWarning(const AbstractAirspace &as) const noexcept {
    return as.IsActive() && Find(as, ids_warning);
  }

  bool IsAcked(const AbstractAirspace &as) const noexcept {
    return (!as.IsActive()) || Find(as, ids_acked);
  }

  bool IsInside(const AbstractAirspace &as) const noexcept {
    return as.IsActive() && Find(as, ids_inside);
  }

private:
  bool Find(const AbstractAirspace& as,
            const StaticArray<const AbstractAirspace *,64> &list) const noexcept {
    return list.contains(&as);
  }
};

#endif
