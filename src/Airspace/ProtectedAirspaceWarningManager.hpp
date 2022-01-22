/* Copyright_License {

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

#ifndef XCSOAR_PROTECTED_AIRSPACE_WARNING_MANAGER_HPP
#define XCSOAR_PROTECTED_AIRSPACE_WARNING_MANAGER_HPP

#include "Engine/Airspace/Ptr.hpp"
#include "thread/Guard.hpp"

#include <optional>

class AirspaceWarning;
class AirspaceWarningManager;
class FlatProjection;

class ProtectedAirspaceWarningManager : public Guard<AirspaceWarningManager> {
public:
  explicit ProtectedAirspaceWarningManager(AirspaceWarningManager &awm) noexcept
    :Guard<AirspaceWarningManager>(awm) {}

  [[gnu::pure]]
  const FlatProjection &GetProjection() const noexcept;

  void Clear() noexcept;
  void AcknowledgeAll() noexcept;

  [[gnu::pure]]
  bool GetAckDay(const AbstractAirspace &airspace) const noexcept;

  void AcknowledgeDay(ConstAirspacePtr airspace, bool set=true) noexcept;
  void AcknowledgeWarning(ConstAirspacePtr airspace, bool set=true) noexcept;
  void AcknowledgeInside(ConstAirspacePtr airspace, bool set=true) noexcept;
  void Acknowledge(ConstAirspacePtr airspace) noexcept;

  [[gnu::pure]]
  bool IsEmpty() const noexcept;

  /**
   * Returns a copy of the highest priority warning, or an empty
   * instance if there is no warning.
   */
  [[gnu::pure]]
  std::optional<AirspaceWarning> GetTopWarning() const noexcept;
};

#endif
