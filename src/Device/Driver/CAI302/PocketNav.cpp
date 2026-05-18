// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PocketNav.hpp"
#include "util/StringFormat.hpp"
#include "Device/Port/Port.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

bool
CAI302::PutMacCready(Port &port, double mc, OperationEnvironment &env)
{
  unsigned mac_cready = uround(Units::ToUserUnit(mc * 10, Unit::KNOTS));

  char buffer[32];
  const int written = StringFormat(buffer, sizeof(buffer), "!g,m%u\r",
                                   mac_cready);
  if (written < 0 || written >= (int)sizeof(buffer))
    return false;

  port.FullWrite(buffer, env, std::chrono::milliseconds{100});
  return true;
}

bool
CAI302::PutBugs(Port &port, double bugs, OperationEnvironment &env)
{
  unsigned bugs2 = uround(bugs * 100);

  char buffer[32];
  const int written = StringFormat(buffer, sizeof(buffer), "!g,u%u\r", bugs2);
  if (written < 0 || written >= (int)sizeof(buffer))
    return false;

  port.FullWrite(buffer, env, std::chrono::milliseconds{100});
  return true;
}

bool
CAI302::PutBallast(Port &port, double fraction, OperationEnvironment &env)
{
  unsigned ballast = uround(fraction * 10);

  char buffer[32];
  const int written = StringFormat(buffer, sizeof(buffer), "!g,b%u\r",
                                   ballast);
  if (written < 0 || written >= (int)sizeof(buffer))
    return false;

  port.FullWrite(buffer, env, std::chrono::milliseconds{100});
  return true;
}
