// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BaseBlackboard.hpp"
#include "Blackboard/ComputerSettingsBlackboard.hpp"

/**
 * Blackboard class used by glide computer (calculation) thread.
 * Can only write DERIVED_INFO
 */
class GlideComputerBlackboard:
  public BaseBlackboard,
  public ComputerSettingsBlackboard
{
  DerivedInfo Finish_Derived_Info;

public:
  void ReadBlackboard(const MoreData &nmea_info);
  void ReadComputerSettings(const ComputerSettings &settings);

protected:
  void ResetFlight(const bool full=true);
  void StartTask();
  void SaveFinish();
  void RestoreFinish();

  // only the glide computer can write to calculated
  DerivedInfo& SetCalculated() { return calculated_info; }
};
