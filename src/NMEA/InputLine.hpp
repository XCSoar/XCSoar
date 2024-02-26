// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/CSVLine.hpp"

struct SpeedVector;
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

  /**
   * Read a #SpeedVector: first an angle [degrees], then the norm [kph
   * == km/h].
   */
  bool ReadSpeedVectorKPH(SpeedVector &value_r) noexcept;

  /**
   * Like ReadSpeedVectorKPH(), but the first column is the norm and
   * the bearing comes after that.  (A weird ordering only used by the
   * Leonardo driver.)
   */
  bool ReadSwappedSpeedVectorKPH(SpeedVector &value_r) noexcept;
};
