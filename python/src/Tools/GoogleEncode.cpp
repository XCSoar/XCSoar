// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
// Original code of Douglas-Peucker algorithm by Robert Coup <robert.coup@koordinates.com>

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

