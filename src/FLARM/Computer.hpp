// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Calculations.hpp"

struct FlarmData;
struct NMEAInfo;

class FlarmComputer {
  FlarmCalculations flarm_calculations;

public:
  /**
   * Calculates location, altitude, average climb speed and
   * looks up the callsign of each target
   */
  void Process(FlarmData &flarm, const FlarmData &last_flarm,
               const NMEAInfo &basic) noexcept;
};
