// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"

#include "Engine/Airspace/AbstractAirspace.hpp"
#include <chrono>
#include <unordered_set>
#include <vector>

class Airspaces;

namespace NOTAMAirspaceSync {

struct Result {
  std::unordered_set<const AbstractAirspace *> injected;
  unsigned added_count = 0;
  unsigned filtered_count = 0;
};

[[nodiscard]]
Result Rebuild(Airspaces &airspaces,
               const std::vector<struct NOTAM> &notams,
               const NOTAMSettings &settings,
               std::chrono::system_clock::time_point now,
               const std::unordered_set<const AbstractAirspace *>
                 &previous_injected);

} // namespace NOTAMAirspaceSync
