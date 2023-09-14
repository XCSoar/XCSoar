// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Operation/Operation.hpp"
#include "Operation/NoCancelOperationEnvironment.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Config.hpp"

void
CAI302Device::CommandMode(OperationEnvironment &env)
{
  if (mode == Mode::COMMAND)
    return;

  port.StopRxThread();

  mode = Mode::UNKNOWN;
  CAI302::CommandMode(port, env);
  mode = Mode::COMMAND;
}

void
CAI302Device::DownloadMode(OperationEnvironment &env)
{
  if (mode == Mode::DOWNLOAD)
    return;

  port.StopRxThread();

  mode = Mode::UNKNOWN;
  CAI302::DownloadMode(port, env);
  mode = Mode::DOWNLOAD;
}

void
CAI302Device::UploadMode(OperationEnvironment &env)
{
  if (mode == Mode::UPLOAD)
    return;

  port.StopRxThread();

  mode = Mode::UNKNOWN;
  CAI302::UploadMode(port, env);
  mode = Mode::UPLOAD;
}

bool
CAI302Device::SetBaudRate(unsigned baud_rate, OperationEnvironment &env)
{
  CommandMode(env);
  CAI302::SetBaudRate(port, baud_rate, env);

  if (!port.Drain())
    return false;

  /* the CAI302 needs some time to apply the new baud rate; if we
     switch too early, it'll cancel the command */
  env.Sleep(std::chrono::milliseconds(500));

  port.SetBaudrate(baud_rate);

  CAI302::CommandMode(port, env);
  return true;
}

bool
CAI302Device::EnableBulkMode(OperationEnvironment &env)
{
  /* this operation should not be cancellable, because we don't know
     what to do with a partial result */
  NoCancelOperationEnvironment env2(env);

  return !config.UsesSpeed() || config.bulk_baud_rate == 0 ||
    config.bulk_baud_rate == config.baud_rate ||
    SetBaudRate(config.bulk_baud_rate, env2);
}

bool
CAI302Device::DisableBulkMode(OperationEnvironment &env)
{
  /* even if the operation was cancelled, switch back to the normal
     baud rate safely */
  NoCancelOperationEnvironment env2(env);

  return !config.UsesSpeed() || config.bulk_baud_rate == 0 ||
    config.bulk_baud_rate == config.baud_rate ||
    SetBaudRate(config.baud_rate, env2);
}

void
CAI302Device::LinkTimeout()
{
  mode = Mode::UNKNOWN;
}

bool
CAI302Device::EnableNMEA(OperationEnvironment &env)
{
  if (mode == Mode::NMEA)
    return true;

  port.StopRxThread();

  mode = Mode::UNKNOWN;
  CAI302::LogMode(port, env);
  mode = Mode::NMEA;
  return true;
}
