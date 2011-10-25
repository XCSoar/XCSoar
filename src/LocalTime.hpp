/*
Copyright_License {

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
#ifndef LOCALTIME_H
#define LOCALTIME_H

#include "Compiler.h"

struct NMEAInfo;

/**
 * Returns the local time as a number of second since midnight 
 * based on info from the device or GPS
 */
gcc_pure
unsigned
DetectCurrentTime(const NMEAInfo &Basic);

/**
 * Returns the local time express as seconds from midnight
 * @param d UTC time in seconds
 * @param utc_offset Offset in second
 */
gcc_pure
unsigned
TimeLocal(int d, int utc_offset);

/**
 * Returns the local time express as seconds from midnight using the 
 * settings to get the UTC offest
 * @param d UTC time in seconds
 */
gcc_pure
unsigned
TimeLocal(int d);

gcc_pure
int
GetUTCOffset(void);

#endif
