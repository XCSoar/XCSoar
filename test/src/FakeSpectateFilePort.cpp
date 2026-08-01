// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Port/OpenSpectateFilePort.hpp"
#include "Device/Port/NullPort.hpp"

std::unique_ptr<Port>
OpenSpectateFilePort(Path, const char *,
                     PortListener *, DataHandler &handler)
{
  return std::make_unique<NullPort>(handler);
}
