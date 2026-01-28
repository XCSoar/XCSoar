// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"

class VolksloggerDevice : public AbstractDevice {
private:
  Port &port;
  unsigned const bulkrate;

public:
  VolksloggerDevice(Port &_port, unsigned const _bulkrate)
                    :port(_port), bulkrate(_bulkrate) {}

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;
  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;
  bool DownloadFlight(const RecordedFlightInfo &flight, Path path,
                      OperationEnvironment &env,
                      unsigned *resume_row = nullptr) override;
};
