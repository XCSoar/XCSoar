// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
// Original code of Douglas-Peucker algorithm by Robert Coup <robert.coup@koordinates.com>

#pragma once

#include <string>
#include <sstream> 
#include <cmath>
#include <utility>
#include <memory>

class GoogleEncode {
private:
  const unsigned dimension;
  const bool delta;
  const double floor_to;

  std::ostringstream encoded;

  unsigned last_dim;
  int *last_values;

private:
  int floor_value(const double value) const {
    return floor(value * floor_to);
  }

  void encodeNumber(unsigned num);

public:
  GoogleEncode(const unsigned _dimension = 1,
               const bool _delta = true,
               const double _floor_to = 1)
    :dimension(_dimension), delta(_delta), floor_to(_floor_to), last_dim(0) {
    last_values = new int[dimension] {};
  }

  ~GoogleEncode() {
    delete[] last_values;
  }

  void addSignedNumber(int num);
  void addUnsignedNumber(unsigned num);
  void addDouble(double num);

  std::unique_ptr<std::string> asString() {
    return std::unique_ptr<std::string>(new std::string(encoded.str()));
  }
};
