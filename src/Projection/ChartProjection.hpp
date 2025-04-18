// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindowProjection.hpp"

class TaskProjection;
class OrderedTaskPoint;

/**
 * Utility class to determine projection for a chart from task data,
 * typically scaled to fill the canvas
 */
class ChartProjection:
  public WindowProjection
{
public:
  ChartProjection() = default;

  explicit ChartProjection(const PixelRect &rc,
                           const TaskProjection &task_projection,
                           double radius_factor=1.1) noexcept
  {
    Set(rc, task_projection, radius_factor);
  }

  ChartProjection(const PixelRect &rc,
                  const OrderedTaskPoint &point) noexcept {
    Set(rc, point);
  }

  void Set(const PixelRect &rc, const TaskProjection &task_projection,
           double radius_factor=1.1) noexcept;

  void Set(const PixelRect &rc, const OrderedTaskPoint &point) noexcept;

private:
  void Set(const PixelRect &rc, const GeoPoint &center, double radius) noexcept;
};
