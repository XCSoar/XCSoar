// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver.hpp"
#include "RadioFrequency.hpp"
#include "TransponderCode.hpp"
#include "system/Path.hpp"

Device::~Device() {}

bool
AbstractDevice::EnableNMEA([[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

void
AbstractDevice::LinkTimeout()
{
}

bool
AbstractDevice::ParseNMEA([[maybe_unused]] const char *line, [[maybe_unused]] struct NMEAInfo &info)
{
  return false;
}

bool
AbstractDevice::PutMacCready([[maybe_unused]] double MacCready, [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutBugs([[maybe_unused]] double bugs, [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutBallast([[maybe_unused]] double fraction, [[maybe_unused]] double overload,
                           [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutQNH([[maybe_unused]] const AtmosphericPressure &pres,
                       [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutVolume([[maybe_unused]] unsigned volume, [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}


bool
AbstractDevice::PutPilotEvent([[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutActiveFrequency([[maybe_unused]] RadioFrequency frequency,
                                   [[maybe_unused]] const TCHAR *name,
                                   [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::PutStandbyFrequency([[maybe_unused]] RadioFrequency frequency,
                                    [[maybe_unused]] const TCHAR *name,
                                    [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::ExchangeRadioFrequencies([[maybe_unused]] OperationEnvironment &env,
                                         [[maybe_unused]] NMEAInfo &info)
{
  return true;
}

bool
AbstractDevice::PutTransponderCode([[maybe_unused]] TransponderCode code,
                                   [[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::EnablePassThrough([[maybe_unused]] OperationEnvironment &env)
{
  return true;
}

bool
AbstractDevice::Declare([[maybe_unused]] const Declaration &declaration, [[maybe_unused]] const Waypoint *home,
                        [[maybe_unused]] OperationEnvironment &env)
{
  return false;
}

void
AbstractDevice::OnSysTicker()
{
}

bool
AbstractDevice::ReadFlightList([[maybe_unused]] RecordedFlightList &flight_list,
                               [[maybe_unused]] OperationEnvironment &env)
{
  return false;
}

bool
AbstractDevice::DownloadFlight([[maybe_unused]] const RecordedFlightInfo &flight,
                               [[maybe_unused]] Path path, [[maybe_unused]] OperationEnvironment &env)
{
  return false;
}

bool
AbstractDevice::DataReceived(std::span<const std::byte>, NMEAInfo &) noexcept
{
  return false;
}
