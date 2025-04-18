// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver/ThermalExpress.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"

class NMEAInputLine;
struct NMEAInfo;

class ThermalExpressDevice : public AbstractDevice {

private:
  bool ParseTXP(NMEAInputLine &line, NMEAInfo &info);

public:
  bool ParseNMEA(const char *line, NMEAInfo &info) override;

};
