// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver/Stratux.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"

class NMEAInputLine;
struct NMEAInfo;

class TimeStamp;
struct TrafficList;

class StratuxDevice : public AbstractDevice {

void
ParsePFLAA(NMEAInputLine &line, TrafficList &flarm, TimeStamp clock) noexcept;

public:
  bool ParseNMEA(const char *line, NMEAInfo &info) override;

};
