// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class TaskManager;

/**
 * This class scans a #TaskManager periodically for changes and
 * generates glide computer events (GCE).
 */
class TaskEventObserver final {
  unsigned best_alternate_id;

public:
  constexpr TaskEventObserver()
    :best_alternate_id(-1) {}

  void Check(const TaskManager &task_manager);
};
