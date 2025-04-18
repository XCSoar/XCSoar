// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GlideRatioCalculator.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"

struct MoreData;
struct DerivedInfo;
struct VarioInfo;

class GlideRatioComputer {
  bool gr_calculator_initialised;

  GlideRatioCalculator gr_calculator;

  GeoPoint last_location;
  double last_altitude;
  Validity last_location_available;

public:
  void Reset();

  void Compute(const MoreData &basic,
               const DerivedInfo &calculated,
               VarioInfo &vario_info,
               const ComputerSettings &settings);
};
