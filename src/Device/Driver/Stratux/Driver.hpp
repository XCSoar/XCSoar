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

  public:

    struct StratuxSettings {
      uint16_t hrange;
      uint16_t vrange;
    };

    bool ParseNMEA(const char *line, NMEAInfo &info) override;
};

void LoadFromProfile(StratuxDevice::StratuxSettings &settings) noexcept;

void SaveToProfile(StratuxDevice::StratuxSettings &settings) noexcept;
