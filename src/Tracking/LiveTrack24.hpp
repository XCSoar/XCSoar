/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
class JobRunner;

namespace LiveTrack24
{
  enum VehicleType {
    VT_PARAGLIDER = 1,
    VT_FLEX_WING_FAI1 = 2,
    VT_RIGID_WING_FAI5 = 4,
    VT_GLIDER = 8,
    VT_PARAMOTOR = 16,
    VT_TRIKE = 32,
    VT_POWERED_AIRCRAFT = 64,
    VT_HOT_AIR_BALLOON = 128,

    VT_WALK = 16385,
    VT_RUN = 16386,
    VT_BIKE = 16388,

    VT_HIKE = 16400,
    VT_CYCLE = 16401,
    VT_MOUNTAIN_BIKE = 16402,
    VT_MOTORCYCLE = 16403,

    VT_WINDSURF = 16500,
    VT_KITESURF = 16501,
    VT_SAILING = 16502,

    VT_SNOWBOARD = 16600,
    VT_SKI = 16601,
    VT_SNOWKITE = 16602,

    VT_CAR = 17100,
    VT_4X4_CAR = 17101,
  };

  typedef uint32_t UserID;
  typedef uint32_t SessionID;

  /**
   * Queries the server for the user id with the given credentials.
   * @param username Case-insensitive username
   * @param password Case-insensitive password
   * @return 0 if userdata are incorrect, or else the userID of the user
   */
  UserID GetUserID(const TCHAR *username, const TCHAR *password);

  /** Generates a random session id */
  SessionID GenerateSessionID();
  /** Generates a random session id containing the given user id */
  SessionID GenerateSessionID(UserID user_id);

  /** Sends the "start of track" packet to the tracking server */
  bool StartTracking(SessionID session, const TCHAR *username,
                     const TCHAR *password, unsigned tracking_interval,
                     VehicleType vtype, const TCHAR *vname = NULL);

  /**
   * Sends a "gps point" packet to the tracking server
   *
   * @param ground_speed Speed over ground in km/h
   */
  bool SendPosition(SessionID session, unsigned packet_id,
                    GeoPoint position, unsigned altitude, unsigned ground_speed,
                    Angle track, int64_t timestamp_utc);

  /** Sends the "end of track" packet to the tracking server */
  bool EndTracking(SessionID session, unsigned packet_id);

  /**
   * Set whether the HTTP requests should be send to
   * the test or the production server
   */
  void SetTestServer(bool use_test_server);
}

#endif
