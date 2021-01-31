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

#ifndef XCSOAR_WARNING_COMPUTER_HPP
#define XCSOAR_WARNING_COMPUTER_HPP

#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "time/DeltaTime.hpp"

class Airspaces;
struct ComputerSettings;
struct MoreData;
struct DerivedInfo;
struct AirspaceWarningsInfo;

/**
 * Manage airspace warnings.
 */
class WarningComputer {
  DeltaTime delta_time;

  Airspaces &airspaces;

  AirspaceWarningManager manager;
  ProtectedAirspaceWarningManager protected_manager;

  bool initialised;

public:
  WarningComputer(const AirspaceWarningConfig &_config,
                  Airspaces &_airspaces);

  ProtectedAirspaceWarningManager &GetManager() {
    return protected_manager;
  }

  const ProtectedAirspaceWarningManager &GetManager() const {
    return protected_manager;
  }

  void Reset() {
    delta_time.Reset();
    initialised = false;
  }

  void Update(const ComputerSettings &settings_computer,
              const MoreData &basic,
              const DerivedInfo &calculated,
              AirspaceWarningsInfo &result);
};

#endif
