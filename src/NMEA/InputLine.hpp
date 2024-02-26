// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/CSVLine.hpp"

class Angle;

/**
 * A helper class which can dissect a NMEA input line.
 */
class NMEAInputLine: public CSVLine {
public:
  explicit NMEAInputLine(const char* line) noexcept;

  /**
   * Parses non-negative floating-point angle value in degrees.
   */
  bool ReadBearing(Angle &value_r) noexcept;
};
