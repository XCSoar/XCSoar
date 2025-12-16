// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Aggregator.hpp"
#include "FLARM/List.hpp"
#include "Math/Angle.hpp"
#include "Math/Util.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

void
TrafficAggregator::FeedFLARM(const FlarmTraffic &flarm, TimeStamp now) noexcept
{
  if (!flarm.IsDefined() || !flarm.valid)
    return;

  UnifiedTraffic unified(flarm);
  unified.source_timestamp = now;

  auto it = traffic_map.find(flarm.id);
  if (it != traffic_map.end()) {
    // Update existing traffic
    // FLARM always wins when present
    it->second = unified;
  } else {
    // Add new traffic
    traffic_map[flarm.id] = unified;
  }
}

void
TrafficAggregator::FeedOGN(UnifiedTraffic::Source source, uint32_t source_id,
                            FlarmId flarm_id, const GeoPoint &location,
                            int altitude, unsigned track, unsigned speed,
                            int climb_rate, TimeStamp timestamp,
                            double turn_rate,
                            FlarmTraffic::AircraftType aircraft_type) noexcept
{
  if (!flarm_id.IsDefined())
    return;

  UnifiedTraffic unified(source, source_id, flarm_id, location,
                         altitude, track, speed, climb_rate, timestamp,
                         turn_rate, aircraft_type);

  auto it = traffic_map.find(flarm_id);
  if (it != traffic_map.end()) {
    // Existing traffic - check if we should replace or merge
    UnifiedTraffic &existing = it->second;

    // If FLARM direct exists and is recent, don't replace with OGN
    if (existing.source == UnifiedTraffic::Source::FLARM_DIRECT) {
      // FLARM always wins - just update timestamp if needed
      if (timestamp > existing.source_timestamp) {
        existing.source_timestamp = timestamp;
      }
      return;
    }

    // Only update if new timestamp is newer or equal
    // This prevents old packets from causing position jumps
    if (timestamp >= existing.source_timestamp) {
      // Merge data from new source
      existing.Update(unified);
    }
    // If timestamp is older, ignore this update to prevent jumping
  } else {
    // Add new traffic
    traffic_map[flarm_id] = unified;
  }
}

void
TrafficAggregator::Expire(TimeStamp now) noexcept
{
  const auto max_age = FloatDuration{MAX_TRAFFIC_AGE.count()};

  for (auto it = traffic_map.begin(); it != traffic_map.end();) {
    const auto age = now - it->second.source_timestamp;
    
    // FLARM traffic expires faster (2 seconds)
    if (it->second.source == UnifiedTraffic::Source::FLARM_DIRECT) {
      if (age > FloatDuration{2.0}) {
        it = traffic_map.erase(it);
        continue;
      }
    } else {
      // Other sources expire after MAX_TRAFFIC_AGE
      if (age > max_age) {
        it = traffic_map.erase(it);
        continue;
      }
    }

    // Check validity
    if (!it->second.valid) {
      it = traffic_map.erase(it);
      continue;
    }

    ++it;
  }
}

TrafficList
TrafficAggregator::GetUnifiedTrafficList(const GeoPoint &own_location,
                                          TimeStamp now) const noexcept
{
  // Zero-initialize to ensure TrivialArray::the_size is 0
  TrafficList result{};
  result.Clear();  // Ensure proper initialization (redundant but safe)
  result.modified.Update(now);

  // Collect and prioritize traffic before adding to list
  struct TrafficEntry {
    UnifiedTraffic traffic;
    double priority;
    
    bool operator<(const TrafficEntry &other) const noexcept {
      return priority > other.priority;  // Higher priority first
    }
  };
  
  std::vector<TrafficEntry> entries;
  entries.reserve(traffic_map.size());

  // Calculate priority for each traffic item
  for (const auto &[id, unified] : traffic_map) {
    if (!unified.IsDefined())
      continue;

    UnifiedTraffic traffic = unified;
    double priority = 0.0;

    // Priority factors:
    // 1. FLARM direct traffic gets highest priority
    if (traffic.source == UnifiedTraffic::Source::FLARM_DIRECT)
      priority += 10000.0;
    
    // 2. Traffic with alarms gets high priority
    if (traffic.HasAlarm())
      priority += 5000.0 + (unsigned)traffic.alarm_level * 1000.0;
    
    // 3. Closer traffic gets higher priority
    if (own_location.IsValid() && traffic.location_available) {
      const double distance = own_location.Distance(traffic.location);
      traffic.distance = RoughDistance(distance);
      priority += 1000.0 / (1.0 + distance / 1000.0);  // Inverse distance
      
      if (traffic.altitude_available) {
        // Calculate relative altitude (simplified - would need own altitude)
        traffic.relative_altitude = traffic.altitude;
      }
    } else {
      traffic.distance = RoughDistance(0);
    }

    entries.push_back({traffic, priority});
  }

  // Sort by priority (highest first)
  std::sort(entries.begin(), entries.end());

  // Add traffic to list (up to MAX_COUNT)
  // Limit to MAX_COUNT to prevent array bounds issues
  const size_t max_add = std::min(entries.size(), 
                                   static_cast<size_t>(TrafficList::MAX_COUNT));
  
  // Double-check that list is empty and properly initialized
  assert(result.list.empty());
  assert(result.list.size() == 0);
  
  for (size_t i = 0; i < max_add; ++i) {
    // Check if list is full before allocating
    if (result.list.full()) {
      break;
    }
    
    FlarmTraffic *flarm = result.AllocateTraffic();
    if (flarm == nullptr) {
      // This should never happen if we checked full() above, but be safe
      break;
    }

    *flarm = entries[i].traffic.ToFlarmTraffic();
    
    // Verify we didn't exceed bounds
    assert(result.list.size() <= TrafficList::MAX_COUNT);
  }
  
  // Final safety check
  assert(result.list.size() <= TrafficList::MAX_COUNT);
  
  LogFormat("GetUnifiedTrafficList: traffic_map_size=%zu entries_size=%zu result_size=%zu",
            traffic_map.size(), entries.size(), static_cast<size_t>(result.list.size()));

  return result;
}

bool
TrafficAggregator::ShouldReplace(const UnifiedTraffic &existing,
                                  const UnifiedTraffic &new_traffic,
                                  const GeoPoint &own_location) const noexcept
{
  // FLARM direct always wins
  if (existing.source == UnifiedTraffic::Source::FLARM_DIRECT)
    return false;

  if (new_traffic.source == UnifiedTraffic::Source::FLARM_DIRECT)
    return true;

  // If new traffic is more recent and from higher priority source
  if ((unsigned)new_traffic.source < (unsigned)existing.source) {
    // Check if we're in FLARM range
    if (own_location.IsValid() && existing.location_available) {
      const double distance = own_location.Distance(existing.location);
      if (distance < FLARM_PRIORITY_RANGE) {
        // In FLARM range - prefer existing if it's FLARM
        return false;
      }
    }
    return true;
  }

  // If same source priority, use most recent
  if (new_traffic.source == existing.source) {
    return new_traffic.source_timestamp > existing.source_timestamp;
  }

  return false;
}

double
TrafficAggregator::CalculateDistance(const UnifiedTraffic &traffic,
                                     const GeoPoint &own_location) const noexcept
{
  if (!own_location.IsValid() || !traffic.location_available)
    return 0;

  return own_location.Distance(traffic.location);
}

