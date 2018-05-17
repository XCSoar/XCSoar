/*
 * Copyright (C) 2012 Tobias Bieniek <Tobias.Bieniek@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TRACKING_GESTURE_MANAGER_HPP
#define TRACKING_GESTURE_MANAGER_HPP

#include "GestureManager.hpp"

#include "Screen/Point.hpp"

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
  const TCHAR* Finish();

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

#endif
