// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/** ICAO ISA sea-level density (kg/m³) */
constexpr double ISA_SEA_LEVEL_DENSITY = 1.225;

/**
 * Calculates the air density from a given QNH-based altitude
 * @param altitude QNH-based altitude (m)
 * @return Air density (kg/m^3)
 */
[[gnu::const]]
double
AirDensity(double altitude) noexcept;

/**
 * Divide TAS by this number to get IAS
 * @param altitude QNH-based altitude (m)
 * @return Ratio of TAS to IAS
 */
[[gnu::const]]
double
AirDensityRatio(double altitude) noexcept;

/**
 * Indicated airspeed from pitot dynamic pressure.
 *
 * IAS = sqrt(2 q / rho0) with q in hPa and rho0 at ISA sea level.
 *
 * @param dynamic_pressure_hpa pitot minus static, hectopascal
 * @return indicated airspeed (m/s)
 */
[[gnu::const]]
double
IndicatedAirspeedFromDynamicPressure(double dynamic_pressure_hpa) noexcept;
