/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in thenv, that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef LIVETRACK24_CLIENT_HPP
#define LIVETRACK24_CLIENT_HPP

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
  Co::Task<UserID> GetUserID(const TCHAR *username, const TCHAR *password);

  /** Sends the "start of track" packet to the tracking server */
  Co::Task<void> StartTracking(SessionID session, const TCHAR *username,
                               const TCHAR *password, unsigned tracking_interval,
                               VehicleType vtype, const TCHAR *vname);

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
  void SetServer(const TCHAR *server) noexcept;

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

#endif
