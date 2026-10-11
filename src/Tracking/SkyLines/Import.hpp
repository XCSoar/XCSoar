// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/ByteOrder.hxx"

#include <chrono>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>

namespace SkyLinesTracking {

constexpr Angle
ImportAngle(int32_t src)
{
  return Angle::Degrees(int32_t(FromBE32(src)) / 1000000.);
}

/**
 * Convert a SkyLines #SkyLinesTracking::GeoPoint to a XCSoar
 * #::GeoPoint.
 */
constexpr ::GeoPoint
ImportGeoPoint(SkyLinesTracking::GeoPoint src)
{
  return ::GeoPoint(ImportAngle(src.longitude), ImportAngle(src.latitude));
}

/**
 * Import a big-endian time stamp to std::chrono::milliseconds.
 */
constexpr std::chrono::milliseconds
ImportTimeMs(uint32_t src_be)
{
  return std::chrono::milliseconds(FromBE32(src_be));
}

/**
 * A #FixPacket as the server receives it.
 */
struct ReceivedFix {
  std::chrono::milliseconds time_of_day;

  /**
   * Invalid if the client did not send one.
   */
  ::GeoPoint location;

  /**
   * Altitude in m above MSL, -1 if the client did not send one.
   */
  int altitude;

  unsigned track_deg;
  bool track_valid;

  /**
   * Ground speed in m/s.
   */
  std::optional<double> ground_speed;

  /**
   * The #FixOGNExtension, if the client sent one.
   */
  struct OGN {
    uint32_t id;
    bool opt_in;

    /**
     * GPS altitude in m above MSL.
     */
    std::optional<int> gps_altitude;

    /**
     * Vertical speed in m/s, not total energy compensated.
     */
    std::optional<double> vertical_speed;
  };

  std::optional<OGN> ogn;
};

/**
 * Import a datagram of type #FIX, with or without #FixOGNExtension.
 *
 * @return std::nullopt if the datagram is too short
 */
[[gnu::pure]]
inline std::optional<ReceivedFix>
ImportFix(std::span<const std::byte> datagram) noexcept
{
  if (datagram.size() < sizeof(FixPacket))
    return std::nullopt;

  /* copy: the datagram buffer is not aligned for the packet */
  FixPacket fix;
  std::memcpy(&fix, datagram.data(), sizeof(fix));
  const uint32_t flags = FromBE32(fix.flags);

  ReceivedFix result;
  result.time_of_day = ImportTimeMs(fix.time);
  result.location = flags & FixPacket::FLAG_LOCATION
    ? ImportGeoPoint(fix.location)
    : ::GeoPoint::Invalid();
  result.altitude = flags & FixPacket::FLAG_ALTITUDE
    ? (int16_t)FromBE16(fix.altitude)
    : -1;
  result.track_valid = flags & FixPacket::FLAG_TRACK;
  result.track_deg = result.track_valid ? FromBE16(fix.track) : 0u;

  if (flags & FixPacket::FLAG_GROUND_SPEED)
    result.ground_speed = FromBE16(fix.ground_speed) / 16.;

  /* an older server ignores the extension; this one ignores a flag
     without the extension */
  if ((flags & FixPacket::FLAG_OGN) &&
      datagram.size() >= sizeof(OGNFixPacket)) {
    FixOGNExtension ogn;
    std::memcpy(&ogn, datagram.data() + sizeof(FixPacket), sizeof(ogn));
    const uint16_t ogn_flags = FromBE16(ogn.flags);

    auto &dest = result.ogn.emplace();
    dest.id = FromBE32(ogn.id);
    dest.opt_in = ogn_flags & FixOGNExtension::FLAG_OPT_IN;

    if (ogn_flags & FixOGNExtension::FLAG_GPS_ALTITUDE)
      dest.gps_altitude = (int16_t)FromBE16(ogn.gps_altitude);

    if (ogn_flags & FixOGNExtension::FLAG_VERTICAL_SPEED)
      dest.vertical_speed = (int16_t)FromBE16(ogn.vertical_speed) / 256.;
  }

  return result;
}

} /* namespace SkyLinesTracking */
