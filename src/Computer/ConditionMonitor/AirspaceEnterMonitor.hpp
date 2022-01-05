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

#pragma once

#include "Engine/Airspace/Ptr.hpp"
#include "util/Serial.hpp"

#include <set>

struct NMEAInfo;
struct DerivedInfo;
struct ComputerSettings;
class ProtectedAirspaceWarningManager;
class AirspaceWarningManager;

/** #ConditionMonitor to track/warn on significant changes in wind speed */
class AirspaceEnterMonitor final {
  const ProtectedAirspaceWarningManager &protected_warnings;

  std::set<ConstAirspacePtr> last_near, last_inside;

  Serial last_serial;

public:
  explicit AirspaceEnterMonitor(const ProtectedAirspaceWarningManager &_warnings) noexcept
    :protected_warnings(_warnings) {}

  void Update(const NMEAInfo &basic, const DerivedInfo &calculated,
              const ComputerSettings &settings) noexcept;

private:
  void Update(const AirspaceWarningManager &warnings) noexcept;
};
