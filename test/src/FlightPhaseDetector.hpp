// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <list>

#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "time/Stamp.hpp"

struct MoreData;
struct DerivedInfo;
enum class CirclingMode : uint8_t;

/**
 * Phase of the flight.
 *
 * May represent both single phase and totals for a set of phases.
 */
struct Phase {

  enum Type : uint8_t {
    NO_PHASE,
    CRUISE,
    CIRCLING,
    POWERED
  };

  enum CirclingDirection : uint8_t {
    NO_DIRECTION,
    LEFT,
    RIGHT,
    MIXED
  };

  /** Type of the phase */
  Type phase_type;
  /** Date and time in UTC when the phase started */
  BrokenDateTime start_datetime;
  /** Date and time in UTC when the phase ended */
  BrokenDateTime end_datetime;
  /** Seconds from midnight UTC when the phase started */
  TimeStamp start_time;
  /** Seconds from midnight UTC when the phase ended */
  TimeStamp end_time;
  /** Direction of circling (or NO_DIRECTION when not circling) */
  CirclingDirection circling_direction;
  /** Starting altitude of the phase */
  double start_alt;
  /** Ending altitude of the phase */
  double end_alt;
  /** Starting location of the phase */
  GeoPoint start_loc;
  /** Ending location of the phase */
  GeoPoint end_loc;
  /** Duration of the phase in seconds */
  FloatDuration duration;
  /** Fraction of the phase duration compared to total flight time. */
  double fraction;
  /** Altitude difference between start_alt and end_alt */
  double alt_diff;
  /** Distance travelled during the phase, i.e. sum of distances between fixes. */
  double distance;
  /** Number of the phases of the same type, combined into this phase. */
  unsigned int merges;

  /** Average ground speed during the phase */
  double GetSpeed() const;
  /** Average vertical speed during the phase */
  double GetVario() const;
  /** Average glide rate during the phase */
  double GetGlideRate() const;

  /**
   * Reinitialize phase
   */
  void Clear() {
    phase_type = NO_PHASE;
    start_datetime.Clear();
    end_datetime.Clear();
    start_time = end_time = TimeStamp::Undefined();
    duration = {};
    fraction = 0;
    circling_direction = NO_DIRECTION;
    alt_diff = 0;
    distance = 0;
    merges = 0;
  }
};

typedef std::list<Phase> PhaseList;


/**
 * Combined statistics for each phase type
 */
struct PhaseTotals {
  Phase total_circstats,
        left_circstats,
        right_circstats,
        mixed_circstats,
        total_cruisestats;

  PhaseTotals() {
    total_circstats.Clear();
    left_circstats.Clear();
    right_circstats.Clear();
    mixed_circstats.Clear();
    total_cruisestats.Clear();
  }
};


/**
 * Detect flight phases
 *
 * Divide flight into circling/cruise phases and calculate basic statistics for
 * each.
 *
 * Dependencies: #CirclingComputer.
 */
class FlightPhaseDetector {
  private:
    Phase previous_phase;
    Phase current_phase;
    int phase_count;
    CirclingMode last_turn_mode;

    PhaseList phases;
    PhaseTotals totals;

    void PushPhase();

  public:
    FlightPhaseDetector();

    /**
     * Split track to circling/cruise phases and calculate basic statistics for
     * each.
     *
     * Actual circling detection is done by #CirclingComputer. We aggregate its
     * data to a list of Phases by grouping set of samples with the same
     * CirclingInfo.turn_mode value.
     *
     * If turn_mode is uncertain for given group (POSSIBLE_CLIMB or
     * POSSIBLE_CRUISE), actual phase type is determined by subsequent group
     * and two groups are combined into single phase. If phase is shorter than
     * certain threshold (MIN_PHASE_TIME), it is not considered as a separate
     * phase and combined with previous one.
     *
     * @param basic Basic flight information for the iteration
     * @param calculated Calculated flight data for the iteration
     */
    void Update(const MoreData &basic, const DerivedInfo &calculated);

    /**
     * Complete phase calculation and calculate overall flight statistics.
     *
     * Called at the end of the flight when no flight data is pending.
     *
     */
    void Finish();

    /**
     * Return detected phases
     *
     * Available after Finish() is called.
     */
    const PhaseList &GetPhases() const {
      return phases;
    }

    /**
     * Return calculated totals
     *
     * Available after Finish() is called.
     */
    const PhaseTotals &GetTotals() const {
      return totals;
    }

    const Phase &GetCurrentPhase() const {
      return current_phase;
    }

};
