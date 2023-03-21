// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"

struct Emulator {
  Port *port;
  DataHandler *handler;
  OperationEnvironment *env;

  virtual ~Emulator() {}
};
