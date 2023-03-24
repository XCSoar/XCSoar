// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*! @file
 * @brief Library for calculating Earth dimensions
 *
 * This library provides general functions for calculating dimensions
 * on the Earth with GPS coordinates.  The implementations are
 * "simplified", i.e. they are fast but not as accurate as they could
 * be.  Instead of using the WGS84 model, they assume the FAI sphere.
 */

#pragma once

struct GeoPoint;
class Angle;

/**
 * Calculates the distance and bearing of two locations
 * @param loc1 Location 1
 * @param loc2 Location 2
 * @param Distance Pointer to the distance variable
 * @param Bearing Pointer to the bearing variable
 */
void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 Angle *distance, Angle *bearing) noexcept;

void
DistanceBearingS(const GeoPoint &loc1, const GeoPoint &loc2,
                 double *distance, Angle *bearing) noexcept;

/**
 * @see FindLatitudeLongitude()
 */
[[gnu::pure]]
GeoPoint
FindLatitudeLongitudeS(const GeoPoint &loc,
                       Angle bearing, double distance) noexcept;

/**
 * @see ProjectedDistance()
 */
[[gnu::pure]]
double
ProjectedDistanceS(const GeoPoint &loc1, const GeoPoint &loc2,
                   const GeoPoint &loc3) noexcept;
