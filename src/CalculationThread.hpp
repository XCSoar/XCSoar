// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/WorkerThread.hpp"
#include "thread/Mutex.hxx"
#include "Computer/Settings.hpp"

class GlideComputer;

/**
 * The CalculationThread handles all expensive calculations
 * that should not be done directly in the device thread.
 * Data transfer is handled by a blackboard system.
 */
class CalculationThread final : public WorkerThread {
  /**
   * This mutex protects #settings_computer and
   * #screen_distance_meters.
   */
  Mutex mutex;

  /**
   * This flag forces a full run of all calculations.  It is set after
   * important non-GPS data has changed, e.g. the task has been
   * edited.
   */
  bool force;

  ComputerSettings settings_computer;

  double screen_distance_meters;

  /** Pointer to the GlideComputer that should be used */
  GlideComputer &glide_computer;

public:
  CalculationThread(GlideComputer &_glide_computer);

  void SetComputerSettings(const ComputerSettings &new_value);
  void SetScreenDistanceMeters(double new_value);

  /**
   * Throws on error.
   */
  void Start(bool suspended=false) {
    WorkerThread::Start(suspended);
    SetLowPriority();
  }

  void ForceTrigger();

protected:
  void Tick() noexcept override;
};
