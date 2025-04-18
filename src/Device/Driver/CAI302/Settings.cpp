// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "PocketNav.hpp"

bool
CAI302Device::PutMacCready(double MacCready, OperationEnvironment &env)
{
  return CAI302::PutMacCready(port, MacCready, env);
}

bool
CAI302Device::PutBugs(double Bugs, OperationEnvironment &env)
{
  return CAI302::PutBugs(port, Bugs, env);
}

bool
CAI302Device::PutBallast(double fraction, [[maybe_unused]] double overload,
                         OperationEnvironment &env)
{
  return CAI302::PutBallast(port, fraction, env);
}
