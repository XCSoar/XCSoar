// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NativePortListener.hpp"
#include "Device/Port/Listener.hpp"

namespace NativePortListener {

void
Initialise()
{
  // No-op
}

void
Deinitialise()
{
  // No-op
}

PortListener *
Create(PortListener &listener)
{
  return &listener;
}

} // namespace NativePortListener
