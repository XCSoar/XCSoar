// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProxyOperationEnvironment.hpp"

bool
ProxyOperationEnvironment::IsCancelled() const noexcept
{
  return other.IsCancelled();
}

void
ProxyOperationEnvironment::SetCancelHandler(std::function<void()> handler) noexcept
{
  other.SetCancelHandler(std::move(handler));
}

void
ProxyOperationEnvironment::Sleep(std::chrono::steady_clock::duration duration) noexcept
{
  other.Sleep(duration);
}

void
ProxyOperationEnvironment::SetErrorMessage(const char *text) noexcept
{
  other.SetErrorMessage(text);
}

void
ProxyOperationEnvironment::SetText(const char *text) noexcept
{
  other.SetText(text);
}

void
ProxyOperationEnvironment::SetProgressRange(unsigned range) noexcept
{
  other.SetProgressRange(range);
}

void
ProxyOperationEnvironment::SetProgressPosition(unsigned position) noexcept
{
  other.SetProgressPosition(position);
}
