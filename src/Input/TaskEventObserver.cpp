// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskEventObserver.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Unordered/AlternateList.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "InputQueue.hpp"

static unsigned
GetBestAlternateID(const TaskManager &tm)
{
  const auto &alternates = tm.GetAlternates();
  return alternates.empty()
    ? unsigned(-1)
    : alternates.front().waypoint->id;
}

void
TaskEventObserver::Check(const TaskManager &tm)
{
  const unsigned new_best_alternate_id = GetBestAlternateID(tm);
  if (new_best_alternate_id != best_alternate_id) {
    best_alternate_id = new_best_alternate_id;
    InputEvents::processGlideComputer(GCE_ALTERNATE_CHANGED);
  }
}
