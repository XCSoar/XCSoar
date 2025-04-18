// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ThermalBand.hpp"

class ThermalEncounterCollection : public ThermalBand {
public:
  void Merge(const ThermalBand &tb) noexcept;

private:
  void MergeUnsafe(const ThermalBand &o) noexcept;
  void LowerFloor(double new_floor) noexcept;
  void UpdateTimes() noexcept;
};

static_assert(std::is_trivial<ThermalEncounterCollection>::value, "type is not trivial");
