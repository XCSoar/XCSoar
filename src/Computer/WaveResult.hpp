// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/Stamp.hpp"
#include "util/TrivialArray.hxx"

struct WaveInfo {
  GeoPoint location;

  GeoPoint a, b;
  Angle normal;

  /**
   * The time (see NMEAInfo::time) when this wave was calculated.
   * This is used to decay old waves.
   *
   * A negative value means a wallclock was not available at the time
   * this wave was calculated.
   */
  TimeStamp time;

  static WaveInfo Undefined() {
    WaveInfo result;
    result.location = GeoPoint::Invalid();
    return result;
  }

  bool IsDefined() const {
    return location.IsValid();
  }

  double GetLength() const {
    return a.DistanceS(b);
  }
};

struct WaveResult {
  TrivialArray<WaveInfo, 32> waves;

  void Clear() {
    waves.clear();
  }
};
