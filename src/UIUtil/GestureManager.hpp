// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"
#include "util/StaticString.hxx"

#include <tchar.h>

/**
 * A manager class that can detect mouse gesture
 * @see http://en.wikipedia.org/wiki/Pointing_device_gesture
 */
class GestureManager
{
  /** Position of the last mouse_move event */
  PixelPoint drag_last;
  /** The gesture string */
  StaticString<11> gesture;

  /** The threshold distance in px for edge detection */
  int threshold;

public:
  /**
   * Constructor of the GestureManager class
   * @param _threshold The threshold distance in px for edge detection
   */
  GestureManager():
    threshold(0) {}

  /**
   * Returns the recognized gesture
   * @return NULL or recognized gesture string
   */
  const char* GetGesture() const;

  /**
   * Stops the GestureManager and returns the recognized gesture
   * @return NULL or recognized gesture string
   */
  const char* Finish();

  /**
   * Starts the GestureManager at the given coordinates
   */
  void Start(PixelPoint p, int _threshold);

  /**
   * Adds new coordinates to the GestureManager
   * @return True if the threshold was reached, False otherwise
   */
  bool Update(PixelPoint p);
};
