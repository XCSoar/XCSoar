/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef TRACE_MANAGER_HPP
#define TRACE_MANAGER_HPP

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

#endif /* TRACE_MANAGER_HPP */

