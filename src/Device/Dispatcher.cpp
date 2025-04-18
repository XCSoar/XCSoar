// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dispatcher.hpp"
#include "Descriptor.hpp"
#include "MultipleDevices.hpp"

bool
DeviceDispatcher::LineReceived(const char *line) noexcept
{
  unsigned i = 0;
  for (DeviceDescriptor *device : devices) {
    if (i++ == exclude)
      /* don't loop back a device's input to itself */
      continue;

    if (device == nullptr)
      continue;

    device->ForwardLine(line);
  }

  return true;
}
