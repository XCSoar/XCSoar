// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"

/**
 * Device driver for IMI ERIXX
 */
class IMIDevice : public AbstractDevice {
private:
  Port &port;

public:
  IMIDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool EnableNMEA(OperationEnvironment &env) override;
  bool ReadFlightList(RecordedFlightList &flight_list,
                      OperationEnvironment &env) override;
  bool DownloadFlight(const RecordedFlightInfo &flight,
                      Path path, OperationEnvironment &env) override;

  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;

private:
  bool Connect(OperationEnvironment &env);
};
