// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cmath>

#if defined(ANDROID) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
/**
 * Helper function for
 * Android Java_org_xcsoar_NonGPSSensors_setBarometricPressure
 * and iOS CMAltimeter.startRelativeAltitudeUpdatesToQueue:
 * Given a current measurement of the atmospheric pressure and the rate
 * of change of atmospheric pressure (in millibars and millibars per
 * second), compute the uncompensated vertical speed of the glider in
 * meters per second, assuming standard atmospheric conditions
 * (deviations from these conditions should not matter very
 * much). This calculation can be derived by taking the formula for
 * converting barometric pressure to pressure altitude (see e.g.
 * http://psas.pdx.edu/RocketScience/PressureAltitude_Derived.pdf),
 * expressing it as a function P(t), the atmospheric pressure at time
 * t, then taking the derivative with respect to t. The dP(t)/dt term
 * is the pressure change rate.
 */
[[gnu::pure]]
static inline double
ComputeNoncompVario(const double pressure, const double d_pressure)
{
  static constexpr double FACTOR(-2260.389548275485);
  static constexpr double EXPONENT(-0.8097374740609689);
  return FACTOR * std::pow(pressure, EXPONENT) * d_pressure;
}
#endif
