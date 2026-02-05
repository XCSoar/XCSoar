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
