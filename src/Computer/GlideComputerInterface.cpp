// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlideComputerInterface.hpp"
#include "GlideComputer.hpp"
#include "Input/InputQueue.hpp"

void
GlideComputerTaskEvents::SetComputer(GlideComputer &_computer) noexcept
{
  computer = &_computer;
}

void
GlideComputerTaskEvents::EnterTransition([[maybe_unused]] const TaskWaypoint &tp) noexcept
{
  computer->OnTransitionEnter();
}

void
GlideComputerTaskEvents::RequestArm([[maybe_unused]] const TaskWaypoint &tp) noexcept
{
  if (!suppressed)
    InputEvents::processGlideComputer(GCE_ARM_READY);
}

void
GlideComputerTaskEvents::ActiveAdvanced([[maybe_unused]] const TaskWaypoint &tp,
                                        [[maybe_unused]] const int i) noexcept
{
  if (!suppressed)
    InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
}

void
GlideComputerTaskEvents::TaskStart() noexcept
{
  if (!suppressed)
    InputEvents::processGlideComputer(GCE_TASK_START);
  computer->OnStartTask();
}

void
GlideComputerTaskEvents::TaskFinish() noexcept
{
  if (!suppressed)
    InputEvents::processGlideComputer(GCE_TASK_FINISH);
}
