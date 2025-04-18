// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MeasurementList.hpp"

struct NMEAInfo;
struct MoreData;
struct DerivedInfo;

/**
 * WindStore receives single windmeasurements and stores these. It uses
 * single measurements to provide a mean value, differentiated for altitude.
 */
class WindStore
{
  double _lastAltitude;
  WindMeasurementList windlist;

  /**
   * The time stamp (NMEAInfo::clock) of the last wind update.  It is
   * used to update DerivedInfo::estimated_wind_available.
   */
  TimeStamp update_clock;

  bool updated;

public:
  /** Clear as if never flown */
  void reset() noexcept;

  /**
   * Called with new measurements. The quality is a measure for how good the
   * measurement is. Higher quality measurements are more important in the
   * end result and stay in the store longer.
   */
  void SlotMeasurement(const MoreData &info,
                       const SpeedVector &wind, unsigned quality) noexcept;

  /**
   * Called if the altitude changes.
   * Determines where measurements are stored and may result in a NewWind
   * signal.
   */
  void SlotAltitude(const MoreData &info, DerivedInfo &derived) noexcept;

  [[gnu::pure]]
  const Vector GetWind(TimeStamp time, double h,
                       bool &found) const noexcept;

private:
  /**
   * Send if a new wind vector has been established. This may happen as
   * new measurements flow in, but also if the altitude changes.
   */
  void NewWind(DerivedInfo &derived, const Vector &wind) const noexcept;

  /**
   * Recalculates the wind from the stored measurements.
   * May result in a NewWind signal.
   */
  void recalculateWind(const MoreData &info, DerivedInfo &derived) const noexcept;
};
