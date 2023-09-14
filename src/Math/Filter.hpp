// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Basic low-pass FIR filter from 2-pole design.
 *
 * @see http://unicorn.us.com/alex/2polefilters.html
 */
class Filter {
  double a[3];
  double b[2];
  double x[3];
  double y[2];

#ifndef NDEBUG
  /** only used for assert() */
  bool ok = false;
#endif

public:
  /**
   * Non-initialising default constructor.  To initialise this
   * instance, call Design().
   */
  Filter() noexcept = default;

  /**
   * Constructor, designs low-pass FIR filter
   *
   * @param cutoff_wavelength 3dB cutoff wavelength (in cycles) of filter design
   * @param bessel If true, generates Bessel filter, otherwise
   * critically damped filter
   */
  Filter(const double cutoff_wavelength, const bool bessel = true) noexcept {
    Design(cutoff_wavelength, bessel);
  }

  /**
   * Designs low-pass FIR filter
   *
   * @param cutoff_wavelength 3dB cutoff wavelength (in cycles) of filter design
   *
   * @return false if failed (cutoff_wavelength too low)
   */
  bool Design(double cutoff_wavelength, bool bessel = true) noexcept;

  /**
   * Resets filter to produce static value
   *
   * @param x0 Steady state value of filter output
   *
   * @return Filter output value
   */
  double Reset(double x0) noexcept;

  /**
   * Updates low-pass filter to calculate filtered output given an input sample
   *
   * @param x0 Input (pre-filtered) value at sample time
   *
   * @return Filter output value
   */
  double Update(double x0) noexcept;
};
