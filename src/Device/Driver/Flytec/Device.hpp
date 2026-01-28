// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "tchar.h"
#include "Device/Driver.hpp"
#include "time/Stamp.hpp"

class Port;
struct NMEAInfo;
class OperationEnvironment;
class RecordedFlightList;
struct RecordedFlightInfo;
class NMEAInputLine;

class FlytecDevice : public AbstractDevice
{
  Port &port;
  TimeStamp last_time = TimeStamp::Undefined();

public:
  FlytecDevice(Port &_port) noexcept:port(_port) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, NMEAInfo &info) override;

  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;

  bool DownloadFlight(const RecordedFlightInfo &flight, Path path,
                      OperationEnvironment &env,
                      unsigned *resume_row = nullptr) override;

private:
  bool ParseFLYSEN(NMEAInputLine &line, NMEAInfo &info);
};
