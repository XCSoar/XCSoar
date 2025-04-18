// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct lua_State;
struct GeoPoint;
class Angle;

namespace Lua {

void
InitGeo(lua_State *L) noexcept;

void
Push(lua_State *L, Angle value);

void
Push(lua_State *L, GeoPoint value);

[[gnu::pure]]
Angle
ToAngle(lua_State *L, int idx);

[[gnu::pure]]
GeoPoint
ToGeoPoint(lua_State *L, int idx);

}
