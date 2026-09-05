// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmThermalComputer.hpp"

#include "FLARM/Calculations.hpp"
#include "NMEA/ThermalProjection.hpp"
#include "Geo/SpeedVector.hpp"
#include "LogFile.hpp"
#include "util/BoundedArray.hxx"

#include <algorithm>
#include <cmath>
#include <tuple>

using namespace FlarmThermalConstants;

FlarmThermalComputer::FlarmThermalComputer() noexcept
  :next_cluster_serial(1),
   last_process_time(TimeStamp::Undefined())
{
  targets.clear();
  clusters.clear();
}

void
FlarmThermalComputer::Reset(TrafficThermalInfo &output) noexcept
{
  targets.clear();
  clusters.clear();
  next_cluster_serial = 1;
  last_process_time = TimeStamp::Undefined();
  output.Clear();
}

FlarmThermalComputer::TargetState *
FlarmThermalComputer::FindTarget(FlarmId id) noexcept
{
  return BoundedArray::FindByKey(
    targets, id,
    [](const TargetState &target) noexcept {
      return target.id;
    });
}

void
FlarmThermalComputer::ResetTargetWindow(TargetState &target) noexcept
{
  target.samples.clear();
  target.assigned_cluster_serial = 0;
  target.qualified = false;
}

void
FlarmThermalComputer::DeactivateTarget(TargetState &target,
                                       const char *reason) noexcept
{
  if (reason == nullptr || *reason == '\0')
    reason = "unknown";

  if (auto *cluster = FindCluster(target.assigned_cluster_serial)) {
    LogDebug("FLARM thermal cluster={} target={:06X} exit={}",
             cluster->serial, target.id.Value(), reason);
    for (auto &contributor : cluster->contributors)
      if (contributor.id == target.id) {
        contributor.active = false;
        break;
      }
  }

  ResetTargetWindow(target);
}

FlarmThermalComputer::TargetState &
FlarmThermalComputer::AllocateTarget(FlarmId id) noexcept
{
  auto allocation = BoundedArray::AppendOrReplaceOldest(
    targets,
    [](const TargetState &target) noexcept {
      return target.last_seen;
    });
  auto &target = allocation.value;
  if (allocation.replaced)
    DeactivateTarget(target, "target-slot-replaced");

  target.id = id;
  target.samples.clear();
  target.assigned_cluster_serial = 0;
  target.last_average_update.Clear();
  target.last_seen = TimeStamp::Undefined();
  target.qualified = false;
  return target;
}

FlarmThermalComputer::ClusterState *
FlarmThermalComputer::FindCluster(std::uint32_t serial) noexcept
{
  if (serial == 0)
    return nullptr;

  return BoundedArray::FindByKey(
    clusters, serial,
    [](const ClusterState &cluster) noexcept {
      return cluster.serial;
    });
}

FlarmThermalComputer::ClusterState &
FlarmThermalComputer::AllocateCluster(TimeStamp first_seen,
                                      double reference_altitude,
                                      TrafficThermalInfo &output) noexcept
{
  auto allocation = BoundedArray::AppendOrReplaceOldest(
    clusters,
    [](const ClusterState &cluster) noexcept {
      return std::tuple{!cluster.closed, cluster.first_seen, cluster.serial};
    });
  auto &cluster = allocation.value;
  if (allocation.replaced) {
    const auto old_serial = cluster.serial;
    for (auto &target : targets)
      if (target.assigned_cluster_serial == old_serial)
        ResetTargetWindow(target);

    output.RemoveBySerial(old_serial);
  }

  do {
    cluster.serial = next_cluster_serial++;
  } while (cluster.serial == 0);

  cluster.contributors.clear();
  cluster.geometry.reference_location = GeoPoint::Invalid();
  cluster.geometry.reference_altitude = reference_altitude;
  cluster.geometry.lift_rate = 0;
  cluster.geometry.wind = SpeedVector::Zero();
  cluster.geometry.drift_per_meter = SpeedVector::Zero();
  cluster.geometry.ground_height = 0;
  cluster.geometry.max_observed_altitude = reference_altitude;
  cluster.first_seen = first_seen;
  cluster.last_seen = first_seen;
  cluster.recent = false;
  cluster.closed = false;
  return cluster;
}

FlarmThermalComputer::ClusterState *
FlarmThermalComputer::FindCompatibleCluster(
    const Candidate &candidate, TimeStamp now) noexcept
{
  ClusterState *best = nullptr;
  double best_distance = GROUPING_RADIUS;

  for (auto &cluster : clusters) {
    bool already_contributed = false;
    for (const auto &contributor : cluster.contributors)
      if (contributor.id == candidate.id) {
        already_contributed = true;
        break;
      }

    if (cluster.contributors.full() && !already_contributed)
      continue;

    const FlarmThermal::ClusterView view{
      cluster.geometry, cluster.first_seen, cluster.last_seen,
      cluster.closed,
    };
    const auto distance =
      FlarmThermal::GetClusterCompatibilityDistance(candidate, view, now);
    if (distance && *distance <= best_distance) {
      best = &cluster;
      best_distance = *distance;
    }
  }

  return best;
}

void
FlarmThermalComputer::UpdateContributor(ClusterState &cluster,
                                        TargetState &target,
                                        const Candidate &candidate,
                                        TimeStamp now) noexcept
{
  ContributorState *contributor = nullptr;
  for (auto &item : cluster.contributors)
    if (item.id == target.id) {
      contributor = &item;
      break;
    }

  if (contributor == nullptr) {
    if (cluster.contributors.full())
      return;

    contributor = &cluster.contributors.append();
    contributor->id = target.id;
    contributor->first_seen = candidate.first_seen;
    contributor->last_seen = now;
    contributor->last_value_time = now;
    contributor->latest_climb_rate = candidate.climb_rate;
    contributor->last_climb_rate = candidate.climb_rate;
    contributor->climb_integral = 0;
    contributor->encounter_duration = 0;
    contributor->encounter_average = candidate.climb_rate;
    contributor->min_altitude = candidate.min_altitude;
    contributor->max_altitude = candidate.max_altitude;
    contributor->reference_altitude = candidate.altitude;
    contributor->geometry_lift_rate = candidate.geometry_lift_rate;
    contributor->geometry_wind = candidate.geometry_wind;
    contributor->drift_per_meter = candidate.drift_per_meter;
  } else if (now > contributor->last_value_time) {
    const auto elapsed = now - contributor->last_value_time;
    if (elapsed <= FlarmCalculations::SAMPLE_POLICY.maximum_gap) {
      const double dt = elapsed.count();
      contributor->climb_integral +=
        (contributor->last_climb_rate + candidate.climb_rate) * 0.5 * dt;
      contributor->encounter_duration += dt;
    }

    contributor->last_value_time = now;
    contributor->last_climb_rate = candidate.climb_rate;
    if (contributor->encounter_duration > 0)
      contributor->encounter_average =
        contributor->climb_integral / contributor->encounter_duration;
    else
      contributor->encounter_average = candidate.climb_rate;
  }

  contributor->last_seen = now;
  contributor->latest_climb_rate = candidate.climb_rate;
  contributor->active = true;

  contributor->source = candidate.source;
  contributor->centre = candidate.centre;
  contributor->reference_altitude = candidate.altitude;
  contributor->min_altitude = std::min(contributor->min_altitude,
                                       candidate.min_altitude);
  contributor->max_altitude = std::max(contributor->max_altitude,
                                       candidate.max_altitude);

  LogDebug("FLARM thermal cluster={} target={:06X} state={} "
           "rolling={:.6f},{:.6f} contributor_ground={:.6f},{:.6f} "
           "altitude={:.1f} reporting_lift={:.2f} geometry_lift={:.2f} "
           "wind={:.1f}@{:.1f}",
           cluster.serial, target.id.Value(),
           target.qualified ? "update" : "qualified",
           candidate.centre.latitude.Degrees(),
           candidate.centre.longitude.Degrees(),
           candidate.source.location.latitude.Degrees(),
           candidate.source.location.longitude.Degrees(),
           candidate.altitude, candidate.climb_rate,
           contributor->geometry_lift_rate,
           contributor->geometry_wind.norm,
           contributor->geometry_wind.bearing.Degrees());

  target.assigned_cluster_serial = cluster.serial;
  target.qualified = true;
  cluster.last_seen = now;
  cluster.recent = false;
  cluster.closed = false;
}

void
FlarmThermalComputer::RecomputeCluster(ClusterState &cluster,
                                       TrafficThermalInfo &output) noexcept
{
  if (cluster.contributors.empty())
    return;

  const auto aggregate = FlarmThermal::CalculateClusterAggregate(
    cluster.contributors, cluster.geometry.reference_altitude);
  cluster.geometry = aggregate.geometry;

  auto &published = output.AllocateSource(cluster.serial);
  published.cluster_serial = cluster.serial;
  published.aircraft_count = cluster.contributors.size();
  published.active_aircraft_count = aggregate.active_count;
  published.min_observed_altitude = aggregate.min_altitude;
  published.max_observed_altitude = aggregate.max_altitude;
  published.first_seen = cluster.first_seen;
  published.last_seen = cluster.last_seen;
  published.active = aggregate.active_count > 0;

  published.reference_location = cluster.geometry.reference_location;
  published.reference_altitude = cluster.geometry.reference_altitude;
  published.geometry_lift_rate = cluster.geometry.lift_rate;
  published.geometry_wind = cluster.geometry.wind;
  published.drift_per_meter = cluster.geometry.drift_per_meter;

  published.thermal.ground_height = cluster.geometry.ground_height;
  published.thermal.location = ProjectThermalCore(
    published.reference_location,
    published.thermal.ground_height - published.reference_altitude,
    published.drift_per_meter);
  published.thermal.lift_rate = aggregate.reporting_lift_rate;
  published.thermal.time = cluster.last_seen;

  LogDebug("FLARM thermal cluster={} ground={:.6f},{:.6f} "
           "reference={:.6f},{:.6f}@{:.1f} reporting_lift={:.2f} "
           "geometry_lift={:.2f} wind={:.1f}@{:.1f} active={}/{}",
           cluster.serial,
           published.thermal.location.latitude.Degrees(),
           published.thermal.location.longitude.Degrees(),
           published.reference_location.latitude.Degrees(),
           published.reference_location.longitude.Degrees(),
           published.reference_altitude,
           published.thermal.lift_rate,
           published.geometry_lift_rate,
           published.geometry_wind.norm,
           published.geometry_wind.bearing.Degrees(),
           aggregate.active_count, cluster.contributors.size());
}

void
FlarmThermalComputer::UpdateLifecycle(TimeStamp now,
                                      TrafficThermalInfo &output) noexcept
{
  for (auto &cluster : clusters) {
    unsigned active_count = 0;
    for (auto &contributor : cluster.contributors) {
      if (contributor.active &&
          (now < contributor.last_seen ||
           now > contributor.last_seen + CONTRIBUTOR_TIMEOUT)) {
        contributor.active = false;
        if (auto *target = FindTarget(contributor.id);
            target != nullptr &&
            target->assigned_cluster_serial == cluster.serial) {
          target->assigned_cluster_serial = 0;
          target->qualified = false;
        }
      }

      if (contributor.active)
        ++active_count;
    }

    if (active_count > 0) {
      cluster.recent = false;
      cluster.closed = false;
    } else if (now >= cluster.last_seen &&
               now <= cluster.last_seen + GROUPING_TIME_GAP) {
      cluster.recent = true;
      cluster.closed = false;
    } else {
      cluster.recent = false;
      cluster.closed = true;
    }

    RecomputeCluster(cluster, output);
  }
}

void
FlarmThermalComputer::MergeClusters(unsigned keep_index,
                                    unsigned remove_index,
                                    TrafficThermalInfo &output) noexcept
{
  auto &keep = clusters[keep_index];
  auto &remove = clusters[remove_index];
  const auto remove_serial = remove.serial;

  for (const auto &incoming : remove.contributors) {
    ContributorState *existing = nullptr;
    for (auto &current : keep.contributors)
      if (current.id == incoming.id) {
        existing = &current;
        break;
      }

    if (existing == nullptr) {
      if (!keep.contributors.full())
        keep.contributors.append(incoming);
      continue;
    }

    *existing = FlarmThermal::MergeContributors(*existing, incoming);
  }

  keep.first_seen = std::min(keep.first_seen, remove.first_seen);
  keep.last_seen = std::max(keep.last_seen, remove.last_seen);
  keep.closed = keep.closed && remove.closed;
  keep.recent = !keep.closed && (keep.recent || remove.recent);

  for (auto &target : targets) {
    if (target.assigned_cluster_serial != remove_serial)
      continue;

    bool retained = false;
    for (const auto &contributor : keep.contributors)
      if (contributor.id == target.id) {
        retained = true;
        break;
      }

    if (retained)
      target.assigned_cluster_serial = keep.serial;
    else
      ResetTargetWindow(target);
  }

  const auto keep_serial = keep.serial;
  output.RemoveBySerial(remove_serial);
  clusters.remove(remove_index);
  if (auto *kept = FindCluster(keep_serial))
    RecomputeCluster(*kept, output);
}

void
FlarmThermalComputer::MergeCompatibleClusters(
    TrafficThermalInfo &output) noexcept
{
  bool merged;
  do {
    merged = false;
    for (unsigned i = 0; i < clusters.size() && !merged; ++i) {
      for (unsigned j = i + 1; j < clusters.size(); ++j) {
        const FlarmThermal::ClusterView a{
          clusters[i].geometry, clusters[i].first_seen,
          clusters[i].last_seen, clusters[i].closed,
        };
        const FlarmThermal::ClusterView b{
          clusters[j].geometry, clusters[j].first_seen,
          clusters[j].last_seen, clusters[j].closed,
        };
        if (!FlarmThermal::AreClustersCompatible(a, b))
          continue;

        unsigned keep = i;
        unsigned remove = j;
        if (!FlarmThermal::IsFirstClusterPreferred(
              clusters[i].first_seen, clusters[i].serial,
              clusters[j].first_seen, clusters[j].serial)) {
          keep = j;
          remove = i;
        }

        MergeClusters(keep, remove, output);
        merged = true;
        break;
      }
    }
  } while (merged);
}

void
FlarmThermalComputer::Process(const TrafficList &traffic, TimeStamp now,
                               double ownship_altitude,
                               const SpeedVector &wind,
                               const RasterTerrain *terrain,
                               TrafficThermalInfo &output) noexcept
{
  if (!now.IsDefined() || !std::isfinite(ownship_altitude) ||
      (last_process_time.IsDefined() && now < last_process_time)) {
    Reset(output);
    if (!now.IsDefined() || !std::isfinite(ownship_altitude))
      return;
  }

  last_process_time = now;
  UpdateLifecycle(now, output);

  for (const auto &item : traffic.list) {
    TargetState *target = FindTarget(item.id);
    if (!FlarmThermal::IsEligibleTraffic(item)) {
      if (target != nullptr)
        DeactivateTarget(*target, "ineligible-traffic");
      continue;
    }

    if (item.climb_rate_avg30s_update ==
          FlarmTraffic::Average30sUpdate::NONE)
      continue;

    if (target == nullptr)
      target = &AllocateTarget(item.id);

    /* The sampling action remains in the copied traffic snapshot until the
       next FLARM calculation pass.  Consume each published action once. */
    if (target->last_average_update &&
        item.valid == target->last_average_update)
      continue;
    target->last_average_update = item.valid;

    if (item.climb_rate_avg30s_reset)
      ResetTargetWindow(*target);

    target->last_seen = now;

    if (item.climb_rate_avg30s_update ==
          FlarmTraffic::Average30sUpdate::IGNORED)
      continue;

    const bool replace_sample =
      item.climb_rate_avg30s_update ==
        FlarmTraffic::Average30sUpdate::REPLACED &&
      !target->samples.empty();

    if (target->samples.full() && !replace_sample)
      target->samples.remove(0);

    auto &sample = replace_sample
      ? target->samples.back()
      : target->samples.append();
    sample.time = now;
    sample.location = item.location;
    // Physical FLARM altitude is relative to ownship.  Rebuild it in the
    // navigation/QNH-or-GPS MSL datum used by terrain and map projection,
    // instead of using FlarmTraffic::altitude's pressure-first datum.
    sample.altitude = item.absolute_altitude
      ? double(item.altitude)
      : ownship_altitude + double(item.relative_altitude);
    if (!std::isfinite(sample.altitude)) {
      DeactivateTarget(*target, "invalid-navigation-altitude");
      continue;
    }
    sample.track = item.track;
    sample.climb_rate = item.climb_rate_avg30s_available
      ? item.climb_rate_avg30s
      : 0;

    while (target->samples.size() > 1 &&
           target->samples.front().time <
             now - FlarmCalculations::AVERAGE_TIME)
      target->samples.remove(0);

    double geometry_lift_rate = item.climb_rate_avg30s;
    SpeedVector geometry_wind = wind;
    if (const auto *assigned = FindCluster(
          target->assigned_cluster_serial))
      for (const auto &contributor : assigned->contributors)
        if (contributor.id == target->id) {
          geometry_lift_rate = contributor.geometry_lift_rate;
          geometry_wind = contributor.geometry_wind;
          break;
        }

    Candidate candidate;
    const auto candidate_result =
      FlarmThermal::BuildCandidate(target->samples, target->id,
                                   target->qualified, item,
                                   geometry_lift_rate, geometry_wind,
                                   terrain, candidate);
    if (candidate_result != CandidateResult::QUALIFIED) {
      if (target->qualified &&
          candidate_result != CandidateResult::INCOMPLETE_WINDOW) {
        DeactivateTarget(
          *target,
          FlarmThermal::GetCandidateResultName(candidate_result));
      }

      continue;
    }

    ClusterState *cluster = FindCluster(target->assigned_cluster_serial);
    if (cluster == nullptr || cluster->closed) {
      cluster = FindCompatibleCluster(candidate, now);
      if (cluster == nullptr)
        cluster = &AllocateCluster(candidate.first_seen,
                                   candidate.altitude, output);
    }

    UpdateContributor(*cluster, *target, candidate, now);
    RecomputeCluster(*cluster, output);
  }

  UpdateLifecycle(now, output);
  MergeCompatibleClusters(output);

  for (unsigned i = 0; i < targets.size();) {
    if (targets[i].last_seen.IsDefined() &&
        now > targets[i].last_seen + CONTRIBUTOR_TIMEOUT) {
      DeactivateTarget(targets[i], "contributor-timeout");
      targets.quick_remove(i);
    } else
      ++i;
  }
}
