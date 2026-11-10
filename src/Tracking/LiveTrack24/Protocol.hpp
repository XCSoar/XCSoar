// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "SessionID.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

struct GeoPoint;

namespace LiveTrack24 {

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

/*
 * LiveTracking API v2, see
 * https://www.livetrack24.com/doc/api/livetracking/LiveTrackingAPIv2.html
 */

/** What every request says about the application. */
struct AppInfo {
  std::string_view key;
  std::string_view version;
  std::string_view device_id;
};

/** One tracking session; a new one per flight. */
struct Session {
  SessionID id = 0;
  UserID user = 0;

  /** MakeToken() with #id */
  std::string token;

  constexpr bool IsDefined() const noexcept {
    return id != 0;
  }
};

/** Sent with the first packet of a session only. */
struct TrackInfo {
  VehicleType category;
  std::string_view vehicle_name;
  std::string_view device_model;
};

struct Fix {
  std::chrono::system_clock::time_point time;
  const GeoPoint &location;

  /** whole metres MSL */
  int altitude;

  /** whole km/h, 0 if unknown */
  unsigned speed;

  /** whole degrees from north, 0 if unknown */
  unsigned course;
};

/** A server reply: `<code>;<message>`, then optional data lines. */
struct Reply {
  unsigned code;
  std::string_view message;
  std::string_view data;

  constexpr bool IsOK() const noexcept {
    return code == 0;
  }
};

/**
 * The password token of the protocol.  The password is lowercased
 * first, as the server does.
 */
std::string
MakeToken(std::string_view password, SessionID session,
          std::string_view app_secret);

[[gnu::pure]]
std::optional<Reply>
ParseReply(std::string_view body) noexcept;

/** The path of the getUserID request, which logs in. */
std::string
LoginPath(const AppInfo &app, std::string_view token,
          std::string_view username);

/** The path of a track request; @p info only on packet 1. */
std::string
TrackPath(const AppInfo &app, const Session &session, unsigned packet_id,
          const Fix &fix, const TrackInfo *info);

/** The path of the trackEnd request, after landing. */
std::string
TrackEndPath(const AppInfo &app, const Session &session,
             unsigned packet_id);

} // namespace Livetrack24
