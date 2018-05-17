/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef LOW_PASS_FILTER_HPP
#define LOW_PASS_FILTER_HPP

#include "Compiler.h"

/**
 * Implements a low-pass filter
 * @see http://en.wikipedia.org/wiki/Low-pass_filter
 * @param y_last Last output value (y-1)
 * @param x_in Input value (x)
 * @param fact Smoothing factor (alpha)
 * @return Output value (y)
 */
gcc_const
static inline double
LowPassFilter(double y_last, double x_in, double fact)
{
  return (1. - fact) * y_last + fact * x_in;
}

#endif
