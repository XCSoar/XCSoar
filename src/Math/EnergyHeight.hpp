// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/Gravity.hpp"
#include "Math/Util.hpp"

/**
 * Kinetic contribution \f$v^2/(2g)\f$ [m], same units as altitude MSL.
 */
[[gnu::const]]
inline constexpr double
Inverse2G() noexcept
{
  return 0.5 / GRAVITY;
}

[[gnu::const]]
inline double
KineticHeight(double true_airspeed_m_s) noexcept
{
  return Square(true_airspeed_m_s) * Inverse2G();
}

/**
 * Navigation (geometric) MSL altitude plus kinetic height from TAS.
 */
[[gnu::const]]
inline double
TotalEnergyHeight(double nav_msl, double true_airspeed_m_s) noexcept
{
  return nav_msl + KineticHeight(true_airspeed_m_s);
}
