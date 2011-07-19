/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef FILTER_HPP
#define FILTER_HPP

#include "Math/fixed.hpp"

/**
 * Basic low-pass FIR filter from 2-pole design.
 *
 * @see http://unicorn.us.com/alex/2polefilters.html
 */
class Filter 
{
  fixed a[3];
  fixed b[2];
  fixed x[3];
  fixed y[2];
  bool m_bessel;
  bool ok;

public:
  /**
   * Constructor, designs low-pass FIR filter
   *
   * @param cutoff_wavelength 3dB cutoff wavelength (in cycles) of filter design
   * @param bessel If true, generates Bessel filter, otherwise
   * critically damped filter
   */
  Filter(const fixed cutoff_wavelength, const bool bessel = true);

  /**
   * Designs low-pass FIR filter
   *
   * @param cutoff_wavelength 3dB cutoff wavelength (in cycles) of filter design
   *
   * @return false if failed (cutoff_wavelength too low)
   */
  bool design(const fixed cutoff_wavelength);

  /**
   * Resets filter to produce static value
   *
   * @param x0 Steady state value of filter output
   *
   * @return Filter output value
   */
  fixed reset(const fixed x0);

  /**
   * Updates low-pass filter to calculate filtered output given an input sample
   *
   * @param x0 Input (pre-filtered) value at sample time
   *
   * @return Filter output value
   */
  fixed update(const fixed x0);

  /**
   * Test whether filter design was successful
   *
   * @return True if design ok
   */
  bool valid() const {
    return ok;
  }
};

#endif
