// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NativeInputListener.hpp"
#include "LogFile.hpp"
#include "io/DataHandler.hpp"

NativeInputListener::NativeInputListener() = default;

bool
NativeInputListener::DataReceived(std::span<const std::byte> s) noexcept
{
  // TODO?
  (void)s;
  return true;
}
