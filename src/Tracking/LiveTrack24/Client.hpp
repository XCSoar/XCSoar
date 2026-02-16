// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Protocol.hpp"
#include "SessionID.hpp"
#include "util/StaticString.hxx"

#include <chrono>
#include <cstdint>

#include <tchar.h>

class Angle;
struct GeoPoint;
class OperationEnvironment;
class CurlGlobal;
class CurlEasy;
namespace Co { template<typename T> class Task; }

namespace LiveTrack24 {

/**
 * API for the LiveTrack24.com server.
 *
 * Procedure:
 *
 * - Generate a Session ID
 *   (including User ID if available)
 *
 * - Send Start-of-Track packet
 *   (on flight start or application start in midair)
 *
 * - Send GPS-Point packet(s)
 *
 * - Send End-of-Track packet
 *   (on landing or application close)
 *
 * @see http://www.livetrack24.com/wiki/en/Leonardo%20Live%20Tracking%20API
 */
class Client final {
  CurlGlobal &curl;

  NarrowString<256> server;

public:
  explicit Client(CurlGlobal &_curl) noexcept
    :curl(_curl) {}

  /**
   * Queries the server for the user id with the given credentials.
   * @param username Case-insensitive username
   * @param password Case-insensitive password
   * @return 0 if userdata are incorrect, or else the userID of the user
   */
  Co::Task<UserID> GetUserID(const char *username, const char *password);

  /** Sends the "start of track" packet to the tracking server */
  Co::Task<void> StartTracking(SessionID session, const char *username,
                               const char *password, unsigned tracking_interval,
                               VehicleType vtype, const char *vname);

  /**
   * Sends a "gps point" packet to the tracking server
   *
   * @param ground_speed Speed over ground in km/h
   */
  Co::Task<void> SendPosition(SessionID session, unsigned packet_id,
                              GeoPoint position, unsigned altitude,
                              unsigned ground_speed,
                              Angle track,
                              std::chrono::system_clock::time_point timestamp_utc);

  /** Sends the "end of track" packet to the tracking server */
  Co::Task<void> EndTracking(SessionID session, unsigned packet_id);

  /**
   * Set the tracking server
   * @param server e.g. www.livetrack24.com (without http:// prefix)
   */
  void SetServer(const char *server) noexcept;

private:
  const char *GetServer() const noexcept {
    return server;
  }

  Co::Task<void> SendRequest(CurlEasy easy);

  /**
   * Throws on error.
   */
  Co::Task<void> SendRequest(const char *url);
};

} // namespace Livetrack24
