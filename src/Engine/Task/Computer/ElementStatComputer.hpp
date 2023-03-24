// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DistanceStatComputer.hpp"
#include "TaskVarioComputer.hpp"

struct ElementStat;

class ElementStatComputer
{
public:
  DistanceStatComputer remaining_effective;
  DistanceStatComputer remaining;
  DistanceStatComputer planned;
  DistanceStatComputer travelled;
  TaskVarioComputer vario;

private:
  bool initialised;

public:
  ElementStatComputer();

  /**
   * Calculate element speeds.  Incremental speeds are
   * held at bulk speeds within first minute of elapsed time.
   *
   * @param time monotonic time of day in seconds
   */
  void CalcSpeeds(ElementStat &data, TimeStamp time) noexcept;

  /**
   * Reset to uninitialised state, to supress calculation
   * of incremental speeds.
   */
  void Reset(ElementStat &data);
};
