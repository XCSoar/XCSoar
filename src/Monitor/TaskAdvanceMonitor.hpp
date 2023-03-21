// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class TaskAdvanceMonitor {
  unsigned last_active_index;
  bool last_need_to_arm;

  friend class TaskAdvanceWidget;
  class TaskAdvanceWidget *widget;

public:
  TaskAdvanceMonitor():widget(nullptr) {}

  void Reset() {
    last_active_index = 0;
    last_need_to_arm = false;
  }

  void Check();
};
