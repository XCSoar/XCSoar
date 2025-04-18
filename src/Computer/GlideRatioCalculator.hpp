// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

static constexpr double INVALID_GR = 999;

struct ComputerSettings;

class GlideRatioCalculator {
  struct Record {
    unsigned distance;
    int altitude;
  };

  /**
   * Rotary array with a predefined max capacity.
   */
  Record records[180];

  unsigned totaldistance;

  /**
   * Pointer to current first item in rotarybuf if used.
   */
  unsigned short start;

  /**
   * Real size of rotary buffer.
   */
  unsigned short size;

  bool valid;

public:
  void Initialize(const ComputerSettings &settings);
  void Add(unsigned distance, int altitude);
  double Calculate() const;
};

// methods using low-pass filter

[[gnu::const]]
double
UpdateGR(double GR, double d, double h, double filter_factor);
