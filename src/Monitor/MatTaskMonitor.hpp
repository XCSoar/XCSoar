// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Monitors the MAT task, watches out for nearby potential turn
 * points, and asks the user whether to add them to the task.
 */
class MatTaskMonitor {
  friend class MatTaskAddWidget;
  class MatTaskAddWidget *widget;

  unsigned last_id;

public:
  MatTaskMonitor():widget(nullptr), last_id(-1) {}

  void Reset() {
    last_id = unsigned(-1);
  }

  void Check();
};
