// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct NMEAInfo;

class Simulator {
public:
  void Init(NMEAInfo &basic);

  /**
   * Update the clock and a few important Validity attributes, as if
   * there had been a new GPS fix, without actually modifying the
   * values.  This is useful to force a calculation update after a
   * simulation parameter has been changed (e.g. altitude).
   */
  void Touch(NMEAInfo &basic);

  void Process(NMEAInfo &basic);
};
