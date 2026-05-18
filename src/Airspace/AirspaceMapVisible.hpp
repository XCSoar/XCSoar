// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Engine/Navigation/Aircraft.hpp"

/**
 * Predicate for map drawing: airspace is shown if the renderer visibility
 * rules say so, or the warning snapshot reports inside/warning for it.
 */
class AirspaceMapVisible {
  const AirspaceVisibility visible_predicate;
  const AirspaceWarningCopy &warnings;

public:
  constexpr
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AircraftState &_state,
                     const AirspaceWarningCopy &_warnings) noexcept
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warnings(_warnings) {}

  [[gnu::pure]]
  bool operator()(const AbstractAirspace &airspace) const noexcept {
    return visible_predicate(airspace) ||
           warnings.IsInside(airspace) ||
           warnings.HasWarning(airspace);
  }
};
