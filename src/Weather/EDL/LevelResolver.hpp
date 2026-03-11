// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Pressure.hpp"

namespace EDL {

struct ResolvedLevel {
  AtmosphericPressure pressure;
  unsigned isobar;

  constexpr unsigned GetHectoPascal() const noexcept {
    return isobar / 100;
  }
};

[[gnu::pure]]
ResolvedLevel
ResolveLevel(AtmosphericPressure qnh, bool qnh_available,
             double altitude) noexcept;

} // namespace EDL
