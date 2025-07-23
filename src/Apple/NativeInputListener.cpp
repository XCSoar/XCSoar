// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NativeInputListener.hpp"
#include "io/DataHandler.hpp"

#include <cstddef>

namespace NativeInputListener {
} // namespace NativeInputListener

void
NativeInputListener::Initialise()
{
}

void
NativeInputListener::Deinitialise()
{
}

DataHandler *
NativeInputListener::Create(DataHandler &handler)
{
  return &handler;
}
