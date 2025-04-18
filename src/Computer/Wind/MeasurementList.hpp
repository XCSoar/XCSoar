// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"
#include "Math/Vector.hpp"
#include "time/Stamp.hpp"

#include <chrono>

/**
 * Structure to hold a single wind measurement
 */
struct WindMeasurement
{
  Vector vector;                /**< Wind speed vector */

  /** Quality of fit */
  unsigned quality;

  /**
   * Time of fix.
   */
  TimeStamp time;
  double altitude;               /**< Altitude of fix */

  constexpr unsigned Score(TimeStamp _time) const noexcept {
    // Calculate the score of this item. The item with the highest
    // score is the least important one.  We may need to adjust the
    // proportion of the quality and the elapsed time. Currently, one
    // quality-point (scale: 1 to 5) is equal to 10 minutes.
    return 600 * (6 - quality) + (_time - time).count();
  }
};

/**
 * The WindMeasurementList is a list that can contain and
 * process windmeasurements.
 */
class WindMeasurementList
{
protected:
  StaticArray<WindMeasurement, 200> measurements;

public:
  /**
   * Returns the weighted mean windvector over the stored values, or 0
   * if no valid vector could be calculated (for instance: too little or
   * too low quality data).
   */
  const Vector getWind(TimeStamp now, double alt,
                       bool &found) const;

  /** Adds the windvector vector with quality quality to the list. */
  void addMeasurement(TimeStamp time,
                      const SpeedVector &vector,
                      double alt, unsigned quality);

  void Reset();

protected:
  /**
   * getLeastImportantItem is called to identify the item that should be
   * removed if the list is too full. Reimplemented from LimitedList.
   */
  [[gnu::pure]]
  unsigned int getLeastImportantItem(TimeStamp now) noexcept;
};
