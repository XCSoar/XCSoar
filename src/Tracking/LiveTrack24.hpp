/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef LIVETRACK24_HPP
#define LIVETRACK24_HPP

#include <tchar.h>
#include <stdint.h>

class Angle;
struct BrokenDateTime;
struct GeoPoint;
class OperationEnvironment;

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
namespace LiveTrack24
{
  enum class VehicleType {
    PARAGLIDER = 1,
    FLEX_WING_FAI1 = 2,
    RIGID_WING_FAI5 = 4,
    GLIDER = 8,
    PARAMOTOR = 16,
    TRIKE = 32,
    POWERED_AIRCRAFT = 64,
    HOT_AIR_BALLOON = 128,

    WALK = 16385,
    RUN = 16386,
    BIKE = 16388,

    HIKE = 16400,
    CYCLE = 16401,
    MOUNTAIN_BIKE = 16402,
    MOTORCYCLE = 16403,

    WINDSURF = 16500,
    KITESURF = 16501,
    SAILING = 16502,

    SNOWBOARD = 16600,
    SKI = 16601,
    SNOWKITE = 16602,

    CAR = 17100,
    CAR_4X4 = 17101,
  };

  typedef uint32_t UserID;
  typedef uint32_t SessionID;

  /**
   * Queries the server for the user id with the given credentials.
   * @param username Case-insensitive username
   * @param password Case-insensitive password
   * @return 0 if userdata are incorrect, or else the userID of the user
   */
  UserID GetUserID(const TCHAR *username, const TCHAR *password,
                   OperationEnvironment &env);

  /** Generates a random session id */
  SessionID GenerateSessionID();
  /** Generates a random session id containing the given user id */
  SessionID GenerateSessionID(UserID user_id);

  /** Sends the "start of track" packet to the tracking server */
  bool StartTracking(SessionID session, const TCHAR *username,
                     const TCHAR *password, unsigned tracking_interval,
                     VehicleType vtype, const TCHAR *vname,
                     OperationEnvironment &env);

  /**
   * Sends a "gps point" packet to the tracking server
   *
   * @param ground_speed Speed over ground in km/h
   */
  bool SendPosition(SessionID session, unsigned packet_id,
                    GeoPoint position, unsigned altitude, unsigned ground_speed,
                    Angle track, int64_t timestamp_utc,
                    OperationEnvironment &env);

  /** Sends the "end of track" packet to the tracking server */
  bool EndTracking(SessionID session, unsigned packet_id,
                   OperationEnvironment &env);

  /**
   * Set the tracking server
   * @param server e.g. www.livetrack24.com (without http:// prefix)
   */
  void SetServer(const TCHAR *server);
}

#endif
