// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct NMEAInfo;
struct DerivedInfo;
struct ComputerSettings;
class Waypoints;

class AutoQNH {
  const unsigned QNH_TIME;

  unsigned countdown_autoqnh;

public:
  constexpr AutoQNH(const unsigned qnh_time = 10)
    : QNH_TIME(qnh_time), countdown_autoqnh(qnh_time)
  {};

  void Process(const NMEAInfo &basic, DerivedInfo &calculated,
               const ComputerSettings &settings_computer,
               const Waypoints &way_points);

  void Reset();

protected:
  bool IsFinished() const {
    return countdown_autoqnh > QNH_TIME;
  }

  bool CalculateQNH(const NMEAInfo &basic, DerivedInfo &calculated,
                    const Waypoints &way_points);
  void CalculateQNH(const NMEAInfo &basic, DerivedInfo &calculated,
                    double altitude);
};
