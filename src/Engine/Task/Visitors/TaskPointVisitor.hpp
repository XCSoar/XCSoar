// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

class TaskPoint;

/**
 * Generic visitor for const task points (for double-dispatch)
 */
class TaskPointConstVisitor {
public:
  virtual void Visit(const TaskPoint &tp) = 0;
};
