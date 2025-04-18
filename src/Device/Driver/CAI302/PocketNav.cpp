// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PocketNav.hpp"
#include "Device/Port/Port.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

#include <stdio.h>

bool
CAI302::PutMacCready(Port &port, double mc, OperationEnvironment &env)
{
  unsigned mac_cready = uround(Units::ToUserUnit(mc * 10, Unit::KNOTS));

  char buffer[32];
  sprintf(buffer, "!g,m%u\r", mac_cready);
  port.FullWrite(buffer, env, std::chrono::milliseconds{100});
  return true;
}

bool
CAI302::PutBugs(Port &port, double bugs, OperationEnvironment &env)
{
  unsigned bugs2 = uround(bugs * 100);

  char buffer[32];
  sprintf(buffer, "!g,u%u\r", bugs2);
  port.FullWrite(buffer, env, std::chrono::milliseconds{100});
  return true;
}

bool
CAI302::PutBallast(Port &port, double fraction, OperationEnvironment &env)
{
  unsigned ballast = uround(fraction * 10);

  char buffer[32];
  sprintf(buffer, "!g,b%u\r", ballast);
  port.FullWrite(buffer, env, std::chrono::milliseconds{100});
  return true;
}
