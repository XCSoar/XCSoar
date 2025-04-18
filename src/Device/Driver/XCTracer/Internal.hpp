// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "time/BrokenDate.hpp"
#include "time/Stamp.hpp"

struct NMEAInfo;
class NMEAInputLine;

class XCTracerDevice final : public AbstractDevice {
  /**
   * time and date of last GPS fix
   * used to check whether date/time has advanced
   */
  TimeStamp last_time = TimeStamp::Undefined();
  BrokenDate last_date = BrokenDate::Invalid();

  /**
   * parser for the LXWP0 sentence
   */
  bool LXWP0(NMEAInputLine &line, NMEAInfo &info);

  /**
   * parser for the XCTRC sentence
   */
  bool XCTRC(NMEAInputLine &line, NMEAInfo &info);

public:
  /**
   * virtual methods from class Device
   */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};
