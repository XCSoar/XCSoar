// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Mutex.hxx"
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

  operator Mutex &() const {
    return const_cast<Mutex &>(mutex);
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
  void LockedCopyTo(TracePointVector &v,
                    std::chrono::duration<unsigned> min_time,
                    const GeoPoint &location, double resolution) const;

  void Update(const ComputerSettings &settings_computer,
              const MoreData &basic, const DerivedInfo &calculated);
};
