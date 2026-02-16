// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Operation/Operation.hpp"
#include "system/Sleep.h"
#include "util/ConvertString.hpp"
#include "util/Exception.hxx"

void
OperationEnvironment::SetError(std::exception_ptr e) noexcept
{
  SetErrorMessage(UTF8ToWideConverter(GetFullMessage(e).c_str()));
}

bool
NullOperationEnvironment::IsCancelled() const noexcept
{
  return false;
}

void
NullOperationEnvironment::SetCancelHandler(std::function<void()>) noexcept
{
  /* this class doesn't support cancellation, so this is a no-op */
}

void
NullOperationEnvironment::Sleep(std::chrono::steady_clock::duration) noexcept
{
}

void
NullOperationEnvironment::SetErrorMessage([[maybe_unused]] const char *text) noexcept
{
}

void
NullOperationEnvironment::SetText([[maybe_unused]] const char *text) noexcept
{
}

void
NullOperationEnvironment::SetProgressRange([[maybe_unused]] unsigned range) noexcept
{
}

void
NullOperationEnvironment::SetProgressPosition([[maybe_unused]] unsigned position) noexcept
{
}

void
QuietOperationEnvironment::Sleep(std::chrono::steady_clock::duration duration) noexcept
{
  ::Sleep(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}
