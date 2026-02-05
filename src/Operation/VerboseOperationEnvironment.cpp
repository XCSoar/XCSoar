// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VerboseOperationEnvironment.hpp"
#include "ProgressGlue.hpp"

void
VerboseOperationEnvironment::SetText(const char *text) noexcept
{
  ProgressGlue::Create(text);
}

void
VerboseOperationEnvironment::SetProgressRange(unsigned range) noexcept
{
  ProgressGlue::SetRange(range);
}

void
VerboseOperationEnvironment::SetProgressPosition(unsigned position) noexcept
{
  ProgressGlue::SetValue(position);
}

void
VerboseOperationEnvironment::UpdateLayout() noexcept
{
  ProgressGlue::Close();
  ProgressGlue::Create(nullptr);
}

void
VerboseOperationEnvironment::Hide() noexcept
{
  ProgressGlue::Close();
}
