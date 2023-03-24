// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataEditor.hpp"
#include "Blackboard/DeviceBlackboard.hpp"

DeviceDataEditor::DeviceDataEditor(DeviceBlackboard &_blackboard,
                                   std::size_t idx) noexcept
  :blackboard(_blackboard), lock(blackboard.mutex),
   basic(blackboard.SetRealState(idx)) {}

void
DeviceDataEditor::Commit() const noexcept
{
  blackboard.ScheduleMerge();
}
