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

  Original code of Douglas-Peucker algorithm by Robert Coup <robert.coup@koordinates.com>
*/

#include "GoogleEncode.hpp"

void GoogleEncode::addSignedNumber(int value) {
  int num;

  if (delta) {
    num = value - last_values[last_dim % dimension];
    last_values[last_dim % dimension] = value;
    last_dim += 1;
  } else {
    num = value;
  }

  unsigned sgn_num = static_cast<unsigned>(num) << 1;

  if (num < 0) {
    sgn_num = ~sgn_num;
  }

  encodeNumber(sgn_num);
}

void GoogleEncode::addUnsignedNumber(unsigned value) {
  unsigned num;

  if (delta) {
    num = value - last_values[last_dim % dimension];
    last_values[last_dim % dimension] = value;
    last_dim += 1;
  } else {
    num = value;
  }

  encodeNumber(num);
}

void GoogleEncode::addDouble(double value) {
  int num;

  if (delta) {
    num = floor_value(value) - last_values[last_dim % dimension];
    last_values[last_dim % dimension] = floor_value(value);
    last_dim += 1;
  } else {
    num = floor_value(value);
  }

  unsigned sgn_num = static_cast<unsigned>(num) << 1;

  if (num < 0) {
    sgn_num = ~sgn_num;
  }

  encodeNumber(sgn_num);
}

void GoogleEncode::encodeNumber(unsigned num) {
  while (num >= 0x20) {
    unsigned next_value = (0x20 | (num & 0x1f)) + 63;
    encoded << char(next_value);
    num >>= 5;
  }

  num += 63;
  encoded << char(num);
}

