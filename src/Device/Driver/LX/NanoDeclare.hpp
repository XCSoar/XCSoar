// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Port;
struct Declaration;
class OperationEnvironment;

namespace Nano {
  bool Declare(Port &port, const Declaration &declaration,
               OperationEnvironment &env);
}
