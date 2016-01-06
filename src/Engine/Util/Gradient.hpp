/* Copyright_License {

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
#ifndef GRADIENT_HPP
#define GRADIENT_HPP

#include "Compiler.h"

/** 
 * Convert angle or (approximate) inverse-gradient to gradient.
 * Where absolute value of gradient is greater than 999 or undefined,
 * the value is limited to 999
 *
 * @param d Angle (radians) or inverse gradient
 * 
 * @return Gradient equivalent to angle
 */
gcc_const
double
AngleToGradient(const double d);

/**
 * Determines whether gradient is error value (999)
 *
 * @param d Gradient
 *
 * @return True if gradient effectively infinite
 */
gcc_const
bool
GradientValid(const double d);

#endif
