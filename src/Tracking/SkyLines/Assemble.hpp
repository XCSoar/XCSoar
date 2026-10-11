// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#include <cstdint>

struct NMEAInfo;
struct MoreData;
struct GeoPoint;
class Angle;

namespace SkyLinesTracking {

struct PingPacket;
struct ACKPacket;
struct FixPacket;
struct OGNFixPacket;
struct Thermal;
struct ThermalSubmitPacket;
struct ThermalRequestPacket;
struct TrafficRequestPacket;
struct UserNameRequestPacket;

[[gnu::const]]
PingPacket
MakePing(uint64_t key, uint16_t id);

[[gnu::const]]
ACKPacket
MakeAck(uint64_t key, uint16_t id, uint32_t flags);

[[gnu::const]]
FixPacket
MakeFix(uint64_t key, uint32_t flags, uint32_t time,
        ::GeoPoint location, Angle track,
        double ground_speed, double airspeed,
        int altitude, double vario, unsigned enl);

[[gnu::pure]]
FixPacket
ToFix(uint64_t key, const NMEAInfo &basic);

/**
 * A fix for the XCSoar Cloud, with what it needs to forward the fix
 * to the Open Glider Network.  Do not send it to SkyLines.
 *
 * @param id the OGN APRS id (see #FixOGNExtension::id), 0 if not
 * known
 * @param opt_in may the fix be forwarded to OGN?
 */
[[gnu::pure]]
OGNFixPacket
ToOGNFix(uint64_t key, const MoreData &basic, uint32_t id, bool opt_in);

[[gnu::const]]
Thermal
MakeThermal(uint32_t time,
            ::GeoPoint bottom_location, int bottom_altitude,
            ::GeoPoint top_location, int top_altitude,
            double lift);

[[gnu::const]]
ThermalSubmitPacket
MakeThermalSubmit(uint64_t key, uint32_t time,
                  ::GeoPoint bottom_location, int bottom_altitude,
                  ::GeoPoint top_location, int top_altitude,
                  double lift);

[[gnu::const]]
ThermalRequestPacket
MakeThermalRequest(uint64_t key);

[[gnu::const]]
TrafficRequestPacket
MakeTrafficRequest(uint64_t key, bool followees, bool club, bool nearby);

[[gnu::const]]
UserNameRequestPacket
MakeUserNameRequest(uint64_t key, uint32_t user_id);

} /* namespace SkyLinesTracking */
