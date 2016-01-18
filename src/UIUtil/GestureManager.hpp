/*
 * Copyright (C) 2003-2010 Tobias Bieniek <Tobias.Bieniek@gmx.de>
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

#ifndef GESTURE_MANAGER_HPP
#define GESTURE_MANAGER_HPP

#include "Util/StaticString.hxx"
#include "Screen/Point.hpp"

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
  const TCHAR* GetGesture() const;

  /**
   * Stops the GestureManager and returns the recognized gesture
   * @return NULL or recognized gesture string
   */
  const TCHAR* Finish();

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

#endif
