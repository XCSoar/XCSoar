/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_PROTECTION_HPP
#define XCSOAR_PROTECTION_HPP

/**
 * Notify the #MergeThread that new data has arrived in the
 * #DeviceBlackboard.
 */
void
TriggerMergeThread();

void TriggerGPSUpdate();

/**
 * Force a #CalculationThread run.  This should be called when
 * important data has been modified, e.g. after the task has been
 * edited.  Even if there is no new GPS position, the task must be
 * recalculated.
 */
void
ForceCalculation();

void TriggerVarioUpdate();

/**
 * Trigger a redraw of the map window.
 */
void
TriggerMapUpdate();

/**
 * Called by the calculation thread when new calculation results are
 * available.  This updates the map and the info boxes.
 */
void
TriggerCalculatedUpdate();

void CreateCalculationThread();

extern bool global_running;

/**
 * Suspend all threads which have unprotected access to shared data.
 * Call this before doing write operations on shared data.
 */
void
SuspendAllThreads();

/**
 * Resume all threads suspended by SuspendAllThreads().
 */
void
ResumeAllThreads();

class ScopeSuspendAllThreads {
public:
  ScopeSuspendAllThreads() { SuspendAllThreads(); }
  ~ScopeSuspendAllThreads() { ResumeAllThreads(); }
};

#endif

