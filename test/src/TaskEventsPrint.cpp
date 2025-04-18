// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskEventsPrint.hpp"

#include <stdio.h>

void
TaskEventsPrint::EnterTransition([[maybe_unused]] const TaskWaypoint &tp) noexcept
{
  if (verbose)
    printf("#- entered sector\n");
}

void
TaskEventsPrint::ExitTransition([[maybe_unused]] const TaskWaypoint &tp) noexcept
{
  if (verbose)
    printf("#- exited sector\n");
}

void
TaskEventsPrint::RequestArm([[maybe_unused]] const TaskWaypoint &tp) noexcept
{
  if (verbose)
    printf("#- ready to advance\n");
}

void
TaskEventsPrint::TaskStart() noexcept
{
  if (verbose)
    printf("#- task started\n");
}

void
TaskEventsPrint::TaskFinish() noexcept
{
  if (verbose)
    printf("#- task finished\n");
}

void
TaskEventsPrint::ActiveAdvanced([[maybe_unused]] const TaskWaypoint &tp, const int i) noexcept
{
  if (verbose)
    printf("#- advance to sector %d\n", i);
}
