// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

/** 
 * Convert angle or (approximate) inverse-gradient to gradient.
 * Where absolute value of gradient is greater than 999 or undefined,
 * the value is limited to 999
 *
 * @param d Angle (radians) or inverse gradient
 * 
 * @return Gradient equivalent to angle
 */
[[gnu::const]]
double
AngleToGradient(const double d) noexcept;

/**
 * Determines whether gradient is error value (999)
 *
 * @param d Gradient
 *
 * @return True if gradient effectively infinite
 */
[[gnu::const]]
bool
GradientValid(const double d) noexcept;
