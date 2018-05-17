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

#ifndef PYTHON_GOOGLEENCODE_HPP
#define PYTHON_GOOGLEENCODE_HPP

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

#endif /* PYTHON_GOOGLEENCODE_HPP */

