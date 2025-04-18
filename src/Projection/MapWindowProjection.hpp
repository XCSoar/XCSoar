// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowProjection.hpp"

struct Waypoint;

class MapWindowProjection:
  public WindowProjection
{
public:
  /**
   * Sets a map scale which is not affected by the hard-coded scale
   * list.
   */
  void SetFreeMapScale(double x) noexcept;

  void SetMapScale(double x) noexcept;

public:
  bool HaveScaleList() const noexcept {
    return true;
  }

  /**
   * Calculates a scale index.
   */
  [[gnu::pure]]
  double CalculateMapScale(unsigned scale) const noexcept;

  [[gnu::pure]]
  double StepMapScale(double scale, int Step) const noexcept;

  [[gnu::pure]]
  bool WaypointInScaleFilter(const Waypoint &way_point) const noexcept;

private:
  double LimitMapScale(double value) const noexcept;

  [[gnu::pure]]
  unsigned FindMapScale(double Value) const noexcept;
};
