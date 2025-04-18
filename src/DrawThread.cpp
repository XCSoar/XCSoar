// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DrawThread.hpp"

#ifndef ENABLE_OPENGL

#include "MapWindow/GlueMapWindow.hpp"
#include "Hardware/CPU.hpp"

/**
 * Main loop of the DrawThread
 */
void
DrawThread::Run() noexcept
{
  SetLowPriority();

  std::unique_lock lock{mutex};

  // circle until application is closed
  while (!_CheckStoppedOrSuspended(lock)) {
    if (!pending) {
      command_trigger.wait(lock);
      continue;
    }

    pending = false;

    const ScopeUnlock unlock(mutex);

#ifdef HAVE_CPU_FREQUENCY
    const ScopeLockCPU cpu;
#endif

    // Get data from the DeviceBlackboard
    map.ExchangeBlackboard();

    // Draw the moving map
    map.Repaint();
  }
}

#endif
