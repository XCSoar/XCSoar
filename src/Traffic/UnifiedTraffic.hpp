// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Traffic.hpp"
#include "FLARM/Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "time/Stamp.hpp"
#include "Rough/RoughAltitude.hpp"
#include "Rough/RoughDistance.hpp"
#include "Rough/RoughSpeed.hpp"
#include "Rough/RoughAngle.hpp"
#include "Math/Angle.hpp"
#include "util/StaticString.hxx"

#include <type_traits>

/**
 * Unified traffic structure that aggregates data from all sources
 * (FLARM, OGN, SkyLines, GliderLink, etc.)
 */
struct UnifiedTraffic {
  /**
   * Traffic source types, ordered by priority (highest to lowest)
   */
  enum class Source : uint8_t {
    FLARM_DIRECT = 0,  // Direct FLARM connection (highest priority)
    GLIDERLINK = 1,    // GliderLink traffic
    OGN_CLOUD = 2,     // OGN via xcsoar-cloud
    SKYLINES = 3,      // SkyLines tracking
    STRATUX = 4,       // Stratux ADS-B
  };

  /** Primary identifier for deduplication */
  FlarmId id;

  /** Source of this traffic data */
  Source source;

  /** Timestamp when this data was received from the source */
  TimeStamp source_timestamp;

  /** Original source-specific ID (e.g., pilot_id for SkyLines) */
  uint32_t source_id;

  /** Location of the traffic */
  GeoPoint location;

  /** Track bearing in degrees (0-359) */
  RoughAngle track;

  /** Ground speed in m/s */
  RoughSpeed speed;

  /** Altitude in meters above MSL */
  RoughAltitude altitude;

  /** Climb rate in m/s */
  double climb_rate;

  /** Average climb rate over 30s */
  double climb_rate_avg30s;

  /** Distance from our aircraft */
  RoughDistance distance;

  /** Relative altitude difference */
  RoughAltitude relative_altitude;

  /** Turn rate in degrees/second */
  double turn_rate;

  /** Aircraft type (from FLARM if available) */
  FlarmTraffic::AircraftType type;

  /** Alarm level (from FLARM if available) */
  FlarmTraffic::AlarmType alarm_level;

  /** Name/callsign of the aircraft */
  StaticString<10> name;

  /** Is the target in stealth mode (FLARM only) */
  bool stealth;

  /** Validity tracking */
  Validity valid;

  /** Has geographical location been calculated? */
  bool location_available;

  /** Was track received from source or calculated? */
  bool track_received;

  /** Was speed received from source or calculated? */
  bool speed_received;

  /** Has absolute altitude been calculated? */
  bool altitude_available;

  /** Was climb rate received from source or calculated? */
  bool climb_rate_received;

  /** Was turn rate received from source or calculated? */
  bool turn_rate_received;

  /** Has averaged climb rate been calculated? */
  bool climb_rate_avg30s_available;

  /** Relative position (for FLARM direct) */
  double relative_north;
  double relative_east;

  UnifiedTraffic() = default;

  /**
   * Create UnifiedTraffic from FLARM traffic
   */
  explicit UnifiedTraffic(const FlarmTraffic &flarm) noexcept
    :id(flarm.id),
     source(Source::FLARM_DIRECT),
     source_timestamp(TimeStamp::Undefined()),  // Will be set by caller
     source_id(0),  // FLARM doesn't have a separate source ID
     location(flarm.location),
     track(flarm.track),
     speed(flarm.speed),
     altitude(flarm.altitude),
     climb_rate(flarm.climb_rate),
     climb_rate_avg30s(flarm.climb_rate_avg30s),
     distance(flarm.distance),
     relative_altitude(flarm.relative_altitude),
     turn_rate(flarm.turn_rate),
     type(flarm.type),
     alarm_level(flarm.alarm_level),
     name(flarm.name),
     stealth(flarm.stealth),
     valid(flarm.valid),
     location_available(flarm.location_available),
     track_received(flarm.track_received),
     speed_received(flarm.speed_received),
     altitude_available(flarm.altitude_available),
     climb_rate_received(flarm.climb_rate_received),
     turn_rate_received(flarm.turn_rate_received),
     climb_rate_avg30s_available(flarm.climb_rate_avg30s_available),
     relative_north(flarm.relative_north),
     relative_east(flarm.relative_east) {}

  /**
   * Create UnifiedTraffic from OGN/SkyLines traffic
   */
  UnifiedTraffic(Source _source, uint32_t _source_id,
                 FlarmId _flarm_id, const GeoPoint &_location,
                 int _altitude, unsigned _track, unsigned _speed,
                 int _climb_rate, TimeStamp _timestamp,
                 double _turn_rate = 0,
                 FlarmTraffic::AircraftType _aircraft_type = FlarmTraffic::AircraftType::UNKNOWN) noexcept
    :id(_flarm_id),
     source(_source),
     source_timestamp(_timestamp),
     source_id(_source_id),
     location(_location),
     track(_track <= 359 ? Angle::Degrees(_track) : Angle::Zero()),
     speed(RoughSpeed(_speed)),
     altitude(RoughAltitude(_altitude)),
     climb_rate(_climb_rate),
     climb_rate_avg30s(0),
     distance(0),
     relative_altitude(0),
     turn_rate(_turn_rate),
     type(_aircraft_type),
     alarm_level(FlarmTraffic::AlarmType::NONE),
     stealth(false),
     valid(_timestamp),
     location_available(true),
     track_received(_track <= 359),  // Track is valid if 0-359 (0 = north is valid)
     speed_received(_speed > 0),
     altitude_available(true),
     climb_rate_received(_climb_rate != 0),
     turn_rate_received(_turn_rate != 0),
     climb_rate_avg30s_available(false),
     relative_north(0),
     relative_east(0) {}

  bool IsDefined() const noexcept {
    return valid && id.IsDefined();
  }

  bool HasAlarm() const noexcept {
    return alarm_level != FlarmTraffic::AlarmType::NONE;
  }

  bool HasName() const noexcept {
    return !name.empty();
  }

  void Clear() noexcept {
    valid.Clear();
    name.clear();
  }

  /**
   * Update from another UnifiedTraffic (merge data)
   * Only updates if the new data is newer or if fields are missing
   */
  void Update(const UnifiedTraffic &other) noexcept {
    // Only update if new timestamp is newer or equal
    if (other.source_timestamp < source_timestamp) {
      // New data is older - don't update location/track/speed
      // But still update alarm level (always use highest)
      if ((unsigned)other.alarm_level > (unsigned)alarm_level) {
        alarm_level = other.alarm_level;
      }
      return;
    }

    // Always update timestamp
    source_timestamp = other.source_timestamp;

    // Update location if available and newer
    if (other.location_available) {
      location = other.location;
      location_available = true;
    }

    // Update altitude if available
    if (other.altitude_available) {
      altitude = other.altitude;
      altitude_available = true;
    }

    // Update track if received from source
    if (other.track_received) {
      track = other.track;
      track_received = true;
    }

    // Update speed if received from source
    if (other.speed_received) {
      speed = other.speed;
      speed_received = true;
    }

    // Update climb rate if received from source
    if (other.climb_rate_received) {
      climb_rate = other.climb_rate;
      climb_rate_received = true;
    }

    // Update turn rate if received from source
    if (other.turn_rate_received) {
      turn_rate = other.turn_rate;
      turn_rate_received = true;
    }

    // Update name if available
    if (other.HasName()) {
      name = other.name;
    }

    // Update type if known
    if (other.type != FlarmTraffic::AircraftType::UNKNOWN) {
      type = other.type;
    }

    // Update alarm level (always use highest)
    if ((unsigned)other.alarm_level > (unsigned)alarm_level) {
      alarm_level = other.alarm_level;
    }

    // Update relative position if available (FLARM only)
    if (other.relative_north != 0 || other.relative_east != 0) {
      relative_north = other.relative_north;
      relative_east = other.relative_east;
    }

    // Update distance if available
    if (!other.distance.IsZero()) {
      distance = other.distance;
    }

    // Update validity
    valid = other.valid;
  }

  /**
   * Convert to FlarmTraffic for compatibility
   */
  FlarmTraffic ToFlarmTraffic() const noexcept {
    FlarmTraffic flarm;
    flarm.id = id;
    flarm.location = location;
    flarm.track = track;
    flarm.speed = speed;
    flarm.altitude = altitude;
    flarm.climb_rate = climb_rate;
    flarm.climb_rate_avg30s = climb_rate_avg30s;
    flarm.distance = distance;
    flarm.relative_altitude = relative_altitude;
    flarm.turn_rate = turn_rate;
    flarm.type = type;
    flarm.alarm_level = alarm_level;
    flarm.name = name;
    flarm.stealth = stealth;
    flarm.valid = valid;
    flarm.location_available = location_available;
    flarm.track_received = track_received;
    flarm.speed_received = speed_received;
    flarm.altitude_available = altitude_available;
    flarm.climb_rate_received = climb_rate_received;
    flarm.turn_rate_received = turn_rate_received;
    flarm.climb_rate_avg30s_available = climb_rate_avg30s_available;
    flarm.relative_north = relative_north;
    flarm.relative_east = relative_east;
    return flarm;
  }
};

static_assert(std::is_trivial<UnifiedTraffic>::value, "type is not trivial");

