// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractReplay.hpp"
#include "Replay/DemoReplay.hpp"

class DeviceBlackboard;
class ProtectedTaskManager;

class DemoReplayGlue
  : public AbstractReplay, private DemoReplay
{
  DeviceBlackboard &device_blackboard;
  ProtectedTaskManager &task_manager;

public:
  DemoReplayGlue(DeviceBlackboard &_device_blackboard,
                 ProtectedTaskManager &_task_manager) noexcept;

  bool Update(NMEAInfo &data) override;
};
