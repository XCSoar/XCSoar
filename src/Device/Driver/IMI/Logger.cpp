// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Protocol/Protocol.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"

bool
IMIDevice::ReadFlightList(RecordedFlightList &flight_list,
                          OperationEnvironment &env)
{
  port.StopRxThread();

  bool success = Connect(env);
  success = success && IMI::ReadFlightList(port, flight_list, env);

  return success;
}

bool
IMIDevice::DownloadFlight(const RecordedFlightInfo &flight, Path path,
                          OperationEnvironment &env, [[maybe_unused]] unsigned *resume_row)
{
  port.StopRxThread();

  bool success = Connect(env);
  success = success && IMI::FlightDownload(port, flight, path, env);

  return success;
}
