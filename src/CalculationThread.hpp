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

#ifndef XCSOAR_CALCULATION_THREAD_HPP
#define XCSOAR_CALCULATION_THREAD_HPP

#include "Thread/WorkerThread.hpp"
#include "Thread/Mutex.hpp"
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

  bool Start(bool suspended=false) {
    if (!WorkerThread::Start(suspended))
      return false;

    SetLowPriority();
    return true;
  }

  void ForceTrigger();

protected:
  virtual void Tick();
};

#endif
