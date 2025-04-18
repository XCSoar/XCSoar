// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class RasterTerrain;
struct GeoPoint;
struct SpeedVector;

void
EstimateThermalBase(const RasterTerrain *terrain,
                    const GeoPoint location, const double altitude,
                    const double average, const SpeedVector wind,
                    GeoPoint &ground_location, double &ground_alt);
