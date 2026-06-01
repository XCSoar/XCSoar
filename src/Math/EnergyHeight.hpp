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

/**
 * Kinetic height convertible to potential energy: excess TAS over best-glide
 * speed, \f$(v^2 - v_{bg}^2)/(2g)\f$ [m].  Negative when slower than best
 * glide.
 */
[[gnu::const]]
inline double
ConvertibleKineticHeight(double true_airspeed_m_s,
                           double best_glide_true_airspeed_m_s) noexcept
{
  return (Square(true_airspeed_m_s) - Square(best_glide_true_airspeed_m_s))
    * Inverse2G();
}

/**
 * Navigation altitude plus convertible kinetic height (final glide / MacCready).
 */
[[gnu::const]]
inline double
GlideConvertibleEnergyHeight(double nav_msl, double true_airspeed_m_s,
                             double best_glide_true_airspeed_m_s) noexcept
{
  return nav_msl + ConvertibleKineticHeight(true_airspeed_m_s,
                                            best_glide_true_airspeed_m_s);
}
