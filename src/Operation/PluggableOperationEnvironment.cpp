// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PluggableOperationEnvironment.hpp"
#include "system/Sleep.h"

bool
PluggableOperationEnvironment::IsCancelled() const noexcept
{
  return other != nullptr && other->IsCancelled();
}

void
PluggableOperationEnvironment::SetCancelHandler(std::function<void()> handler) noexcept
{
  if (other != nullptr)
    other->SetCancelHandler(std::move(handler));
}

void
PluggableOperationEnvironment::Sleep(std::chrono::steady_clock::duration duration) noexcept
{
  if (other != nullptr)
    other->Sleep(duration);
  else
    ::Sleep(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

void
PluggableOperationEnvironment::SetErrorMessage(const char *text) noexcept
{
  if (other != nullptr)
    other->SetErrorMessage(text);
}

void
PluggableOperationEnvironment::SetText(const char *text) noexcept
{
  if (other != nullptr)
    other->SetText(text);
}

void
PluggableOperationEnvironment::SetProgressRange(unsigned range) noexcept
{
  if (other != nullptr)
    other->SetProgressRange(range);
}

void
PluggableOperationEnvironment::SetProgressPosition(unsigned position) noexcept
{
  if (other != nullptr)
    other->SetProgressPosition(position);
}
