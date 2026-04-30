// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Mutex.hxx"
#include "Engine/Trace/Trace.hpp"
#include "util/OverwritingRingBuffer.hpp"

#include <vector>

struct ComputerSettings;
struct MoreData;
struct DerivedInfo;

/**
 * High-rate netto vario sample from MergeThread (between GPS trace fixes).
 */
struct TrailVarioSample {
  TracePoint::Time time{};
  float vario{};
};

/**
 * Record a trace of the current flight.
 */
class TraceComputer {
  /**
   * Capacity for #merge_vario_samples (must match ring template size).
   */
  static constexpr unsigned MERGE_VARIO_SAMPLES_CAPACITY = 512;

  /**
   * This mutex protects trace_full: it must be locked while editing
   * the trace, and while reading it from a thread other than the
   * #CalculationThread.
   */
  mutable Mutex mutex;

  Trace full, contest, sprint;

  /**
   * Netto vario samples at merge frequency for snail-trail colouring
   * between stored GPS trace points.
   */
  OverwritingRingBuffer<TrailVarioSample, MERGE_VARIO_SAMPLES_CAPACITY>
      merge_vario_samples;

  /**
   * Append merge-time vario ring contents to \a vario_samples (mutex must
   * already be held).
   */
  void CopyMergeVarioSamplesUnlocked(
      std::vector<TrailVarioSample> &vario_samples) const;

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
   * Atomically copy full trace and merge-time vario samples under one
   * lock (consistent snapshot for rendering).
   */
  void LockedCopySnapshot(TracePointVector &v,
                          std::vector<TrailVarioSample> &vario_samples) const;

  /**
   * Extract some trace points.  The trace is locked, and the method
   * may be called from any thread.
   */
  void LockedCopyTo(TracePointVector &v,
                    std::chrono::duration<unsigned> min_time,
                    const GeoPoint &location, double resolution) const;

  /**
   * Atomically copy filtered trace points and merge-time vario samples
   * under one lock (consistent snapshot for rendering).
   */
  void LockedCopySnapshot(TracePointVector &v,
                          std::vector<TrailVarioSample> &vario_samples,
                          std::chrono::duration<unsigned> min_time,
                          const GeoPoint &location, double resolution) const;

  /**
   * Called from #MergeThread after merged basic data is computed (must
   * not be called while holding #DeviceBlackboard::mutex).
   */
  void PushMergeVarioSample(TracePoint::Time time, float vario) noexcept;

  void Update(const ComputerSettings &settings_computer,
              const MoreData &basic, const DerivedInfo &calculated);
};
