// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlarmThermalCandidate.hpp"
#include "FlarmThermalCluster.hpp"
#include "FLARM/List.hpp"
#include "NMEA/TrafficThermal.hpp"
#include "NMEA/Validity.hpp"
#include "time/Stamp.hpp"
#include "util/TrivialArray.hxx"

#include <cstdint>

class RasterTerrain;
struct SpeedVector;

/**
 * Calculation state for detecting and grouping thermal climbs reported by
 * physical FLARM traffic.
 *
 * The detector state intentionally stays outside DerivedInfo; only the
 * bounded, aggregate TrafficThermalInfo snapshot crosses the blackboard
 * boundary.
 */
class FlarmThermalComputer {
  using Sample = FlarmThermal::Sample;
  using Candidate = FlarmThermal::Candidate;
  using CandidateResult = FlarmThermal::CandidateResult;
  using ContributorState = FlarmThermal::Contributor;
  using ClusterGeometry = FlarmThermal::ClusterGeometry;

  struct TargetState {
    FlarmId id;
    TrivialArray<Sample, FlarmThermalConstants::MAX_SAMPLE_COUNT> samples;
    std::uint32_t assigned_cluster_serial;
    Validity last_average_update;
    TimeStamp last_seen;
    bool qualified;
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

  void UpdateContributor(ClusterState &cluster, TargetState &target,
                         const Candidate &candidate,
                         TimeStamp now) noexcept;
  void UpdateLifecycle(TimeStamp now,
                       TrafficThermalInfo &output) noexcept;
  void RecomputeCluster(ClusterState &cluster,
                        TrafficThermalInfo &output) noexcept;
  void MergeCompatibleClusters(TrafficThermalInfo &output) noexcept;
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
