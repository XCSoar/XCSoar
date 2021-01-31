/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_OS_CLOCK_HPP
#define XCSOAR_OS_CLOCK_HPP

#include "util/Compiler.h"

#include <cstdint>

/**
 * Returns the value of a monotonic clock in milliseconds.
 */
gcc_pure
unsigned
MonotonicClockMS();

/**
 * Returns the value of a monotonic clock in microseconds.
 */
gcc_pure
uint64_t
MonotonicClockUS();

/**
 * Returns the value of a monotonic clock in seconds as a floating
 * point value.
 */
gcc_pure
double
MonotonicClockFloat();

/**
 * Query the UTC offset from the OS.
 *
 * @return the offset in seconds
 */
gcc_pure
int
GetSystemUTCOffset();

#endif
