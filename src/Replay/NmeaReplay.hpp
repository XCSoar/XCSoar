// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AbstractReplay.hpp"
#include "time/ReplayClock.hpp"
#include "Device/Port/NullPort.hpp"

#include <memory>

class NLineReader;
class NMEAParser;
class Device;
struct DeviceConfig;
struct NMEAInfo;

class NmeaReplay: public AbstractReplay
{
  std::unique_ptr<NLineReader> reader;

  NMEAParser *parser;
  NullPort port;
  Device *device;

  ReplayClock clock;

public:
  NmeaReplay(std::unique_ptr<NLineReader> &&_reader,
             const DeviceConfig &config);
  ~NmeaReplay();

  bool Update(NMEAInfo &data) override;

protected:
  bool ParseLine(const char *line, NMEAInfo &data);

private:
  bool ReadUntilRMC(NMEAInfo &data);
};
