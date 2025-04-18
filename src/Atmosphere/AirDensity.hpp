// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
