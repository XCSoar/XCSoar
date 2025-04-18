// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Port;
class OperationEnvironment;

namespace CAI302 {
  bool PutMacCready(Port &port, double mc, OperationEnvironment &env);
  bool PutBugs(Port &port, double bugs, OperationEnvironment &env);
  bool PutBallast(Port &port, double fraction, OperationEnvironment &env);
}
