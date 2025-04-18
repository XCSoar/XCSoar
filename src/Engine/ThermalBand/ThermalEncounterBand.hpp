// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ThermalBand.hpp"
#include "time/Stamp.hpp"

#include <type_traits>

class ThermalEncounterBand : public ThermalBand
{
  TimeStamp time_start;

public:
  void Reset() noexcept {
    ThermalBand::Reset();
    time_start = TimeStamp::Undefined();
  }

  void AddSample(const TimeStamp time,
                 const double height) noexcept;

private:
  unsigned ResizeToHeight(const double height) noexcept;

  unsigned FindPenultimateFinished(const unsigned index,
                                   const FloatDuration time) noexcept;

  FloatDuration EstimateTimeStep(const FloatDuration time,
                                 const double height,
                                 const unsigned index) noexcept;

  void Start(TimeStamp time, double height) noexcept;

};

static_assert(std::is_trivial<ThermalEncounterBand>::value, "type is not trivial");
