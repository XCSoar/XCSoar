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

#ifndef XCSOAR_TRACKING_SKYLINES_ASSEMBLE_HPP
#define XCSOAR_TRACKING_SKYLINES_ASSEMBLE_HPP

#include "Features.hpp"
#include "Compiler.h"

#include <stdint.h>

struct NMEAInfo;
struct GeoPoint;
class Angle;

namespace SkyLinesTracking {

struct PingPacket;
struct ACKPacket;
struct FixPacket;
struct Thermal;
struct ThermalSubmitPacket;
struct ThermalRequestPacket;
struct TrafficRequestPacket;
struct UserNameRequestPacket;

gcc_const
PingPacket
MakePing(uint64_t key, uint16_t id);

gcc_const
ACKPacket
MakeAck(uint64_t key, uint16_t id, uint32_t flags);

gcc_const
FixPacket
MakeFix(uint64_t key, uint32_t flags, uint32_t time,
        ::GeoPoint location, Angle track,
        double ground_speed, double airspeed,
        int altitude, double vario, unsigned enl);

gcc_pure
FixPacket
ToFix(uint64_t key, const NMEAInfo &basic);

gcc_const
Thermal
MakeThermal(uint32_t time,
            ::GeoPoint bottom_location, int bottom_altitude,
            ::GeoPoint top_location, int top_altitude,
            double lift);

gcc_const
ThermalSubmitPacket
MakeThermalSubmit(uint64_t key, uint32_t time,
                  ::GeoPoint bottom_location, int bottom_altitude,
                  ::GeoPoint top_location, int top_altitude,
                  double lift);

gcc_const
ThermalRequestPacket
MakeThermalRequest(uint64_t key);

gcc_const
TrafficRequestPacket
MakeTrafficRequest(uint64_t key, bool followees, bool club, bool near);

gcc_const
UserNameRequestPacket
MakeUserNameRequest(uint64_t key, uint32_t user_id);

} /* namespace SkyLinesTracking */

#endif
