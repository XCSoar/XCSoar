// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractReplay.hpp"
#include "Replay/DemoReplay.hpp"

class ProtectedTaskManager;

class DemoReplayGlue
  : public AbstractReplay, private DemoReplay
{
  ProtectedTaskManager* task_manager;

public:
  DemoReplayGlue(ProtectedTaskManager &_task_manager);

  bool Update(NMEAInfo &data) override;
};
