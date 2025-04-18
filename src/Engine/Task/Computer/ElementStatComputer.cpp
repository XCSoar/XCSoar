// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ElementStatComputer.hpp"
#include "Task/Stats/ElementStat.hpp"

ElementStatComputer::ElementStatComputer()
  :remaining_effective(),
   remaining(),
   planned(),
   travelled(false),
   vario(),
   initialised(false) {}

void
ElementStatComputer::Reset(ElementStat &data)
{
  initialised = false;

  CalcSpeeds(data, TimeStamp::Undefined());
}

void 
ElementStatComputer::CalcSpeeds(ElementStat &data,
                                const TimeStamp time) noexcept
{
  remaining_effective.CalcSpeed(data.remaining_effective,
                                data.time_remaining_start);
  remaining.CalcSpeed(data.remaining, data.time_remaining_now);
  planned.CalcSpeed(data.planned, data.time_planned);
  travelled.CalcSpeed(data.travelled, data.time_elapsed);

  if (!initialised) {
    if (data.time_elapsed > std::chrono::seconds{15})
      initialised = true;

    vario.reset(data.vario, data.solution_remaining);
    remaining_effective.ResetIncrementalSpeed(data.remaining_effective);
    remaining.ResetIncrementalSpeed(data.remaining);
    planned.ResetIncrementalSpeed(data.planned);
    travelled.ResetIncrementalSpeed(data.travelled);
    return;
  }

  remaining.CalcIncrementalSpeed(data.remaining, time);
  planned.CalcIncrementalSpeed(data.planned, time);
  travelled.CalcIncrementalSpeed(data.travelled, time);

  if (data.solution_remaining.IsOk()) {
    remaining_effective.CalcIncrementalSpeed(data.remaining_effective, time);
    vario.update(data.vario, data.solution_remaining);
  } else {
    remaining_effective.ResetIncrementalSpeed(data.remaining_effective);
    vario.reset(data.vario, data.solution_remaining);
  }
}
