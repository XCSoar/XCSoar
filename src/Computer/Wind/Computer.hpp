// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CirclingWind.hpp"
#include "WindEKFGlue.hpp"
#include "Store.hpp"

struct WindSettings;
class GlidePolar;
struct NMEAInfo;
struct MoreData;
struct DerivedInfo;

/**
 * Calculate the wind vector.
 *
 * Dependencies: #FlyingComputer, #CirclingComputer.
 */
class WindComputer {
  CirclingWind circling_wind;
  WindEKFGlue wind_ekf;

  // TODO: protect with a Mutex
  WindStore wind_store;

  /**
   * The EKF algorithm is available, which skips WindStore and
   * CirclingWind.
   */
  bool ekf_active;

public:
  const WindStore &GetWindStore() const {
    return wind_store;
  }

  void Reset();

  void Compute(const WindSettings &settings,
               const GlidePolar &glide_polar,
               const MoreData &basic, DerivedInfo &calculated);

  void ComputeHeadWind(const NMEAInfo &basic, DerivedInfo &calculated);

  /**
   * Select one of the wind values and write it into
   * DerivedInfo::wind, according to the configuration.
   */
  void Select(const WindSettings &settings,
              const NMEAInfo &basic, DerivedInfo &calculated);
};
