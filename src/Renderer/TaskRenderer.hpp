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
  GeoBounds screen_bounds;

public:
  TaskRenderer(TaskPointRenderer &_tpv, GeoBounds _screen_bounds);

  void Draw(const TaskInterface &task);
  void Draw(const AbortTask &task);
  void Draw(const OrderedTask &task);
  void Draw(const GotoTask &task);
};
