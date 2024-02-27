// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "time/Stamp.hpp"
#include "util/TrivialArray.hxx"

#include <type_traits>

struct SpeedVector;

/** Structure to hold information on identified thermal sources on the ground */
struct ThermalSource
{
  GeoPoint location;
  double ground_height;
  double lift_rate;
  TimeStamp time;

  [[nodiscard]] [[gnu::pure]]
  GeoPoint CalculateAdjustedLocation(double altitude,
                                     const SpeedVector &wind) const noexcept;
};

/** Structure for current thermal estimate from ThermalLocator */
struct ThermalLocatorInfo
{
  static constexpr unsigned MAX_SOURCES = 20;

  /** Location of thermal at aircraft altitude */
  GeoPoint estimate_location;
  /** Is thermal estimation valid? */
  bool estimate_valid;

  /** Position and data of the last thermal sources */
  TrivialArray<ThermalSource, MAX_SOURCES> sources;

  void Clear() noexcept;

  /**
   * Allocate a new #THERMAL_SOURCE_INFO slot; discard the oldest one
   * if the list is full.
   */
  [[nodiscard]]
  ThermalSource &AllocateSource() noexcept;
};

static_assert(std::is_trivial<ThermalLocatorInfo>::value,
              "type is not trivial");
