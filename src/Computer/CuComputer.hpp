// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/CuSonde.hpp"

struct NMEAInfo;
struct DerivedInfo;
struct ComputerSettings;

/**
 * Wrapper for CuSonde.
 */
class CuComputer {
  CuSonde cu_sonde;

public:
  const CuSonde &GetCuSonde() const {
    return cu_sonde;
  }

  void Reset();

  void Compute(const NMEAInfo &basic, const DerivedInfo &calculated,
               const ComputerSettings &settings);
};
