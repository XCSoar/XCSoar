/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

/**
 * Basic low-pass FIR filter from 2-pole design
 */
class Filter 
{
public:
/** 
 * Constructor, designs low-pass FIR filter
 * 
 * @param cutoff_wavelength 3dB cutoff wavelength (in cycles) of filter design
 * @param bessel If true, generates Bessel filter, otherwise critically damped filter
 */
  Filter(const double cutoff_wavelength,
    const bool bessel=true);

/** 
 * Resets filter to produce static value
 * 
 * @param x0 Steady state value of filter output
 * 
 * @return Filter output value
 */
  double reset(const double x0);

/** 
 * Updates low-pass filter to calculate filtered output given an input sample
 * 
 * @param x0 Input (pre-filtered) value at sample time
 * 
 * @return Filter output value
 */
  double update(const double x0);

private:
  double a[3];
  double b[2];
  double x[3];
  double y[2];
};


#endif
