// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PopupOperationEnvironment.hpp"
#include "Message.hpp"

void
PopupOperationEnvironment::SetErrorMessage(const TCHAR *text) noexcept
{
  Message::AddMessage(text);
}
