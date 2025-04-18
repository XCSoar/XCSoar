// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Serial.hpp"
#include "Trace/Trace.hpp"
#include "Trace/Vector.hpp"
#include "Trace/Point.hpp"

class TraceManager {
protected:
  const Trace &trace_master;

private:
  /**
   * This attribute tracks Trace::GetAppendSerial().  It is updated
   * when appnew copy of the master Trace is obtained, and is used to
   * check if that copy should be replaced with a new one.
   */
  Serial append_serial;

  /**
   * This attribute tracks Trace::GetModifySerial().  It is updated
   * when a new copy of the master Trace is obtained, and is used to
   * check if that copy should be replaced with a new one.
   */
  Serial modify_serial;

protected:
  /**
   * Working trace for solver.  This contains pointers to trace_master
   * records, which get Invalidated when the trace gets thinned.  Be
   * careful!
   */
  TracePointerVector trace;

  /** Number of points in current trace set */
  unsigned n_points;

  TracePoint predicted;

  static constexpr unsigned predicted_index = 0xffff;

  bool trace_dirty;

public:
  /**
   * Constructor
   *
   * @param _trace Trace object reference to use for solving
   */
  explicit TraceManager(const Trace &_trace) noexcept;

  /**
   * Sets the location of the "predicted" finish location.  If
   * defined, then the algorithm will assume that you will reach it.
   * Pass an "invalid" #TracePoint to disable this prediction (see
   * TracePoint::Invalid()).
   *
   * @return true if the object was reset
   */
  bool SetPredicted(const TracePoint &_predicted) noexcept;

protected:
  void ClearTrace() noexcept;

  /**
   * Obtain a new #Trace copy.
   */
  void UpdateTraceFull() noexcept;

  /**
   * Copy points that were added to the end of the master Trace.
   *
   * @return true if new points were added
   */
  bool UpdateTraceTail() noexcept;

  [[gnu::pure]]
  const TracePoint &GetPoint(unsigned i) const noexcept {
    assert(i < n_points);

    return *trace[i];
  }

  [[gnu::pure]]
  bool IsMasterUpdated(bool continuous) const noexcept;

  [[gnu::pure]]
  bool CheckMasterSerial() const noexcept {
    return modify_serial != trace_master.GetModifySerial();
  }

  [[gnu::pure]]
  bool IsMasterAppended() const noexcept {
    return append_serial == trace_master.GetAppendSerial();
  }

protected:
  /**
   * Update working trace from master.
   *
   * @param force disable lazy updates, force the trace to be up to
   * date before returning
   */
  virtual void UpdateTrace(bool force) noexcept;

  virtual void Reset() noexcept = 0;
};
