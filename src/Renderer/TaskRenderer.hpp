// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoBounds.hpp"

class TaskPointRenderer;
class AbortTask;
class OrderedTask;
class GotoTask;
class TaskInterface;

class TaskRenderer
{
  TaskPointRenderer &tpv;
  const GeoBounds screen_bounds;

public:
  constexpr TaskRenderer(TaskPointRenderer &_tpv,
                         const GeoBounds &_screen_bounds) noexcept
    :tpv(_tpv), screen_bounds(_screen_bounds) {}

  void Draw(const TaskInterface &task) noexcept;
  void Draw(const AbortTask &task) noexcept;
  void Draw(const OrderedTask &task) noexcept;
  void Draw(const GotoTask &task) noexcept;
};
