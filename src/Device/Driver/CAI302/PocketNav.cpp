// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PocketNav.hpp"
#include "Device/Port/Port.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

#include <stdio.h>

static bool
PutGCommand(Port &port, const char *command, unsigned value,
            OperationEnvironment &env)
{
  char buffer[32];
  sprintf(buffer, "!g,%s%u\r", command, value);
  port.FullWrite(buffer, env, std::chrono::milliseconds{100});
  return true;
}

bool
CAI302::PutMacCready(Port &port, double mc, OperationEnvironment &env)
{
  unsigned mac_cready = uround(Units::ToUserUnit(mc * 10, Unit::KNOTS));
  return PutGCommand(port, "m", mac_cready, env);
}

bool
CAI302::PutBugs(Port &port, double bugs, OperationEnvironment &env)
{
  unsigned bugs2 = uround(bugs * 100);
  return PutGCommand(port, "u", bugs2, env);
}

bool
CAI302::PutBallast(Port &port, double fraction, OperationEnvironment &env)
{
  unsigned ballast = uround(fraction * 10);
  return PutGCommand(port, "b", ballast, env);
}
