/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Internal.hpp"
#include "Protocol.hpp"
#include "Operation/Operation.hpp"
#include "Operation/NoCancelOperationEnvironment.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Config.hpp"

bool
CAI302Device::CommandMode(OperationEnvironment &env)
{
  if (mode == Mode::COMMAND)
    return true;

  port.StopRxThread();

  if (CAI302::CommandMode(port, env)) {
    mode = Mode::COMMAND;
    return true;
  } else {
    mode = Mode::UNKNOWN;
    return false;
  }
}

bool
CAI302Device::DownloadMode(OperationEnvironment &env)
{
  if (mode == Mode::DOWNLOAD)
    return true;

  port.StopRxThread();

  if (CAI302::DownloadMode(port, env)) {
    mode = Mode::DOWNLOAD;
    return true;
  } else {
    mode = Mode::UNKNOWN;
    return false;
  }
}

bool
CAI302Device::UploadMode(OperationEnvironment &env)
{
  if (mode == Mode::UPLOAD)
    return true;

  port.StopRxThread();

  if (CAI302::UploadMode(port, env)) {
    mode = Mode::UPLOAD;
    return true;
  } else {
    mode = Mode::UNKNOWN;
    return false;
  }
}

bool
CAI302Device::SetBaudRate(unsigned baud_rate, OperationEnvironment &env)
{
  if (!CommandMode(env) || !CAI302::SetBaudRate(port, baud_rate, env) ||
      !port.Drain())
    return false;

  /* the CAI302 needs some time to apply the new baud rate; if we
     switch too early, it'll cancel the command */
  env.Sleep(500);

  return port.SetBaudrate(baud_rate) && CAI302::CommandMode(port, env);
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

  if (CAI302::LogMode(port, env)) {
    mode = Mode::NMEA;
    return true;
  } else {
    mode = Mode::UNKNOWN;
    return false;
  }
}
