// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/CSVLine.hpp"

/**
 * A helper class which can dissect a NMEA input line.
 */
class NMEAInputLine: public CSVLine {
public:
  explicit NMEAInputLine(const char* line) noexcept;
};
