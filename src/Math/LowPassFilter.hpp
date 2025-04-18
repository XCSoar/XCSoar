// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Implements a low-pass filter
 * @see http://en.wikipedia.org/wiki/Low-pass_filter
 * @param y_last Last output value (y-1)
 * @param x_in Input value (x)
 * @param fact Smoothing factor (alpha)
 * @return Output value (y)
 */
constexpr double
LowPassFilter(double y_last, double x_in, double fact) noexcept
{
  return (1. - fact) * y_last + fact * x_in;
}
