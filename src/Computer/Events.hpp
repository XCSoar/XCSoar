// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListener.hpp"
#include "NMEA/Validity.hpp"

/**
 * This class listens for #LiveBlackboard changes and emits glide
 * computer events.
 *
 * @see InputEvents::processGlideComputer()
 */
class GlideComputerEvents final : public NullBlackboardListener {
  bool enable_team, last_teammate_in_sector;

  bool last_flying;
  bool last_circling;
  bool last_final_glide;

  unsigned last_traffic;
  Validity last_new_traffic;

public:
  GlideComputerEvents():enable_team(false) {}

  void Reset();

  /* methods from BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated);
  virtual void OnComputerSettingsUpdate(const ComputerSettings &settings);
};
