// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#pragma once

#include "GestureManager.hpp"
#include "ui/dim/Point.hpp"

#include <vector>

/**
 * A manager class that can detect mouse gesture and saves
 * the mouse pointer locations for trail rendering
 * @see http://en.wikipedia.org/wiki/Pointing_device_gesture
 */
class TrackingGestureManager: public GestureManager
{
public:
  typedef std::vector<PixelPoint> PointVector;

private:
  PointVector points;

  /** Lower bound for #trail_spacing in pixels */
  static constexpr int MIN_TRAIL_SPACING = 1;

  /**
   * Minimum distance in pixels between two trail points.  This is
   * much smaller than the gesture detection threshold, so the trail
   * follows the finger closely instead of being a coarse polyline
   * through the detection points.
   */
  int trail_spacing = MIN_TRAIL_SPACING;

public:
  /**
   * Stops the GestureManager and returns the recognized gesture
   * @return NULL or recognized gesture string
   */
  const char* Finish();

  /**
   * Starts the GestureManager at the given coordinates
   */
  void Start(PixelPoint p, int threshold);

  /**
   * Adds new coordinates to the GestureManager
   * @return True if the threshold was reached, False otherwise
   */
  bool Update(PixelPoint p);

  /**
   * Returns if there are at least two points for trail rendering
   */
  bool HasPoints() const {
    return points.size() >= 2;
  }

  /**
   * Returns the vector of mouse pointer locations for trail rendering
   */
  const PointVector &GetPoints() const {
    return points;
  }
};
