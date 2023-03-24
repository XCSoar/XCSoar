// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct WindowStats {
  /**
   * The duration of this window [seconds].  A negative value means
   * this object is undefined.
   */
  double duration;

  /**
   * The distance travelled in this window.
   */
  double distance;

  /**
   * The quotient of distance and duration.
   */
  double speed;

  void Reset() {
    duration = -1;
  }
};
