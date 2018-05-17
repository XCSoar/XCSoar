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

#ifndef XCSOAR_TRACE_COMPUTER_HPP
#define XCSOAR_TRACE_COMPUTER_HPP

#include "Thread/Mutex.hpp"
#include "Engine/Trace/Trace.hpp"

struct ComputerSettings;
struct MoreData;
struct DerivedInfo;

/**
 * Record a trace of the current flight.
 */
class TraceComputer {
  /**
   * This mutex protects trace_full: it must be locked while editing
   * the trace, and while reading it from a thread other than the
   * #CalculationThread.
   */
  mutable Mutex mutex;

  Trace full, contest, sprint;

public:
  TraceComputer();

  void Lock() const {
    mutex.Lock();
  }

  void Unlock() const {
    mutex.Unlock();
  }

  /**
   * Returns a reference to the full trace.  When using this reference
   * outside of the #CalculationThread, the mutex must be locked.
   */
  const Trace &GetFull() const {
    return full;
  }

  /**
   * Returns an unprotected reference to the contest trace.  This
   * object may be used only inside the #CalculationThread.
   */
  const Trace &GetContest() const {
    return contest;
  }

  /**
   * Returns an unprotected reference to the sprint trace.  This
   * object may be used only inside the #CalculationThread.
   */
  const Trace &GetSprint() const {
    return sprint;
  }

  void Reset();

  /**
   * Extract all trace points.  The trace is locked, and the method
   * may be called from any thread.
   */
  void LockedCopyTo(TracePointVector &v) const;

  /**
   * Extract some trace points.  The trace is locked, and the method
   * may be called from any thread.
   */
  void LockedCopyTo(TracePointVector &v, unsigned min_time,
                            const GeoPoint &location, double resolution) const;

  void Update(const ComputerSettings &settings_computer,
              const MoreData &basic, const DerivedInfo &calculated);
};

#endif
