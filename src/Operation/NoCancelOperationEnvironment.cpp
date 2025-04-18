// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NoCancelOperationEnvironment.hpp"
#include "system/Sleep.h"

bool
NoCancelOperationEnvironment::IsCancelled() const noexcept
{
  return false;
}

void
NoCancelOperationEnvironment::SetCancelHandler(std::function<void()>) noexcept
{
}

void
NoCancelOperationEnvironment::Sleep(std::chrono::steady_clock::duration duration) noexcept
{
  /* some OperationEnvironment implementations may ignore Sleep()
     calls when the operation is cancelled; override that */

  ::Sleep(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}
