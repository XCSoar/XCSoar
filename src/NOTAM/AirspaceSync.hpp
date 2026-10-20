// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"

#include <chrono>
#include <vector>

class Airspaces;

namespace NOTAMAirspaceSync {

/**
 * Replace downloaded NOTAM areas. File airspaces stay.
 * NOTAM areas are matched by source, not by pointer.
 */
void
Rebuild(Airspaces &airspaces,
        const std::vector<struct NOTAM> &notams,
        const NOTAMSettings &settings,
        std::chrono::system_clock::time_point now);

} // namespace NOTAMAirspaceSync
