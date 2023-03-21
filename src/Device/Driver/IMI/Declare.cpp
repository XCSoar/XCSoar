// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Protocol/Protocol.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Declaration.hpp"
#include "Operation/Operation.hpp"

#include <stdexcept>

bool
IMIDevice::Declare(const Declaration &declaration,
                   [[maybe_unused]] const Waypoint *home,
                   OperationEnvironment &env)
{
  // verify WP number
  unsigned size = declaration.Size();
  if (size < 2)
    throw std::runtime_error("Task is too small");

  if (size > 13)
    throw std::runtime_error("Task is too large");

  port.StopRxThread();

  if (!Connect(env))
    return false;

  IMI::DeclarationWrite(port, declaration, env);
  return true;
}
