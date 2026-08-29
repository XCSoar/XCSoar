// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/List.hpp"
#include "Geo/SpeedVector.hpp"
#include "Math/Angle.hpp"
#include "NMEA/TrafficThermal.hpp"
#include "NMEA/Validity.hpp"
#include "time/Stamp.hpp"
#include "util/TrivialArray.hxx"

#include <cstdint>

class RasterTerrain;
struct SpeedVector;

/** Initial tunable values for the FLARM thermal detector. */
namespace FlarmThermalConstants {
static constexpr FloatDuration CONTRIBUTOR_TIMEOUT{10};
static constexpr FloatDuration GROUPING_TIME_GAP{120};
static constexpr FloatDuration EXIT_TURN_WINDOW{5};

static constexpr double ENTER_CLIMB_THRESHOLD = 0.5;
static constexpr double EXIT_CLIMB_THRESHOLD = 0.3;
static constexpr double MAX_DRIFT_CORRECTED_RADIUS = 500;
static constexpr double GROUPING_RADIUS = 500;
static constexpr double MIN_ACCUMULATED_TURN = 270;
static constexpr double MIN_RECENT_TURN_RATE = 4;
static constexpr double MIN_CURRENT_TURN_RATE = 3;

/** Enough for the complete window after rate-aware sample coalescing. */
static constexpr unsigned MAX_SAMPLE_COUNT = 128;
}

/**
 * Calculation state for detecting and grouping thermal climbs reported by
 * physical FLARM traffic.
 *
 * The detector state intentionally stays outside DerivedInfo; only the
 * bounded, aggregate TrafficThermalInfo snapshot crosses the blackboard
 * boundary.
 */
class FlarmThermalComputer {
  struct Sample {
    TimeStamp time;
    GeoPoint location;
    double altitude;
    Angle track;
    double climb_rate;
  };

  struct TargetState {
    FlarmId id;
    TrivialArray<Sample, FlarmThermalConstants::MAX_SAMPLE_COUNT> samples;
    std::uint32_t assigned_cluster_serial;
    Validity last_average_update;
    TimeStamp last_seen;
    bool qualified;
  };

  struct ContributorState {
    FlarmId id;
    GeoPoint centre;
    ThermalSource source;
    TimeStamp first_seen;
    TimeStamp last_seen;
    TimeStamp last_value_time;
    double latest_climb_rate;
    double last_climb_rate;
    double climb_integral;
    double encounter_duration;
    double encounter_average;
    double min_altitude;
    double max_altitude;
    double reference_altitude;
    double geometry_lift_rate;
    SpeedVector geometry_wind;
    SpeedVector drift_per_meter;
    bool active;
  };

  /**
   * Identity-free aggregate used for detector-side grouping and merging.
   *
   * This deliberately mirrors only the published geometry needed by the
   * detector.  TrafficThermalInfo is an output snapshot and must not become
   * an input to qualification, clustering, or contributor lifecycle rules.
   */
  struct ClusterGeometry {
    GeoPoint reference_location;
    double reference_altitude;
    double lift_rate;
    SpeedVector wind;
    SpeedVector drift_per_meter;
    double ground_height;
    double max_observed_altitude;
  };

  struct ClusterState {
    std::uint32_t serial;
    TrivialArray<ContributorState, TrafficList::DEVICE_MAX_COUNT>
      contributors;
    ClusterGeometry geometry;
    TimeStamp first_seen;
    TimeStamp last_seen;
    bool recent;
    bool closed;
  };

  TrivialArray<TargetState, TrafficList::DEVICE_MAX_COUNT> targets;
  TrivialArray<ClusterState, TrafficThermalInfo::MAX_SOURCES> clusters;
  std::uint32_t next_cluster_serial;
  TimeStamp last_process_time;

  struct Candidate {
    FlarmId id;
    GeoPoint centre;
    ThermalSource source;
    TimeStamp first_seen;
    double altitude;
    double min_altitude;
    double max_altitude;
    double climb_rate;
    double geometry_lift_rate;
    SpeedVector geometry_wind;
    SpeedVector drift_per_meter;
  };

  enum class CandidateResult : std::uint8_t {
    QUALIFIED,
    INCOMPLETE_WINDOW,
    WEAK_LIFT,
    INSUFFICIENT_TURN,
    LEFT_CIRCLE,
    EXCESSIVE_RADIUS,
    INVALID_SOURCE,
  };

  TargetState *FindTarget(FlarmId id) noexcept;
  TargetState &AllocateTarget(FlarmId id) noexcept;
  void ResetTargetWindow(TargetState &target) noexcept;
  void DeactivateTarget(TargetState &target, const char *reason) noexcept;

  ClusterState *FindCluster(std::uint32_t serial) noexcept;
  ClusterState &AllocateCluster(TimeStamp first_seen,
                                double reference_altitude,
                                TrafficThermalInfo &output) noexcept;
  ClusterState *FindCompatibleCluster(const Candidate &candidate,
                                      TimeStamp now) noexcept;

  CandidateResult BuildCandidate(const TargetState &target,
                                 const FlarmTraffic &traffic,
                                 double geometry_lift_rate,
                                 const SpeedVector &geometry_wind,
                                 const RasterTerrain *terrain,
                                 Candidate &candidate) const noexcept;
  void UpdateContributor(ClusterState &cluster, TargetState &target,
                         const Candidate &candidate,
                         TimeStamp now) noexcept;
  void UpdateLifecycle(TimeStamp now,
                       TrafficThermalInfo &output) noexcept;
  void RecomputeCluster(ClusterState &cluster,
                        TrafficThermalInfo &output) noexcept;
  void MergeCompatibleClusters(TimeStamp now,
                               TrafficThermalInfo &output) noexcept;
  void MergeClusters(unsigned keep_index, unsigned remove_index,
                     TrafficThermalInfo &output) noexcept;

public:
  FlarmThermalComputer() noexcept;

  /** Reset detector state and published FLARM thermal history. */
  void Reset(TrafficThermalInfo &output) noexcept;

  /**
   * Process one calculation snapshot.
   *
   * @param ownship_altitude navigation altitude (QNH/barometric MSL when
   * enabled, otherwise GPS MSL), used as the geometry datum for relative
   * physical-FLARM altitudes
   */
  void Process(const TrafficList &traffic, TimeStamp now,
               double ownship_altitude,
               const SpeedVector &wind, const RasterTerrain *terrain,
               TrafficThermalInfo &output) noexcept;
};
