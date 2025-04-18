// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

/**
 * Base class for blackboards, providing read access to NMEA_INFO and DERIVED_INFO
 */
class BaseBlackboard
{
protected:
  MoreData gps_info;
  DerivedInfo calculated_info;

public:
  // all blackboards can be read as const
  constexpr const MoreData &Basic() const noexcept {
    return gps_info;
  }

  constexpr const DerivedInfo& Calculated() const noexcept {
    return calculated_info;
  }
};
