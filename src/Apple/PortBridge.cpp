// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PortBridge.hpp"
#include "NativeInputListener.hpp"
#include "NativePortListener.hpp"

#include <span>
#include <string.h>

PortBridge::PortBridge() {}

void
PortBridge::setListener(PortListener *_listener)
{
  (void)_listener;
  // TODO
}

void
PortBridge::setInputListener(DataHandler *handler)
{
  (void)handler;
  // TODO
}

std::size_t
PortBridge::write(std::span<const std::byte> src)
{
  (void)src;
  // TODO
  return -1;
}
