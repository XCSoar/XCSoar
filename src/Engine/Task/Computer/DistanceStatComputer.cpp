// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DistanceStatComputer.hpp"
#include "Task/Stats/DistanceStat.hpp"

void
DistanceStatComputer::CalcSpeed(DistanceStat &data,
                                FloatDuration time) noexcept
{
  if (time.count() > 0 && data.IsDefined())
    data.speed = data.GetDistance() / time.count();
  else
    data.speed = 0;
}
