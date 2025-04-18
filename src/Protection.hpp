// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Notify the #MergeThread that new data has arrived in the
 * #DeviceBlackboard.
 */
void
TriggerMergeThread() noexcept;

void
TriggerGPSUpdate() noexcept;

/**
 * Force a #CalculationThread run.  This should be called when
 * important data has been modified, e.g. after the task has been
 * edited.  Even if there is no new GPS position, the task must be
 * recalculated.
 */
void
ForceCalculation() noexcept;

void
TriggerVarioUpdate() noexcept;

/**
 * Trigger a redraw of the map window.
 */
void
TriggerMapUpdate() noexcept;

/**
 * Called by the calculation thread when new calculation results are
 * available.  This updates the map and the info boxes.
 */
void
TriggerCalculatedUpdate() noexcept;

void
CreateCalculationThread() noexcept;

extern bool global_running;

/**
 * Suspend all threads which have unprotected access to shared data.
 * Call this before doing write operations on shared data.
 */
void
SuspendAllThreads() noexcept;

/**
 * Resume all threads suspended by SuspendAllThreads().
 */
void
ResumeAllThreads() noexcept;

class ScopeSuspendAllThreads {
public:
  ScopeSuspendAllThreads() noexcept { SuspendAllThreads(); }
  ~ScopeSuspendAllThreads() noexcept { ResumeAllThreads(); }
};

