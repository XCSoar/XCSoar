// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
class WindowProjection;
struct GeoPoint;
struct MapLook;

void
DrawDistanceRings(Canvas &canvas,
                  const WindowProjection &projection,
                  const GeoPoint &aircraft_pos,
                  const MapLook &look) noexcept;
