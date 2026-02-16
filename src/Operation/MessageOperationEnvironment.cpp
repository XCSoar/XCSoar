// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MessageOperationEnvironment.hpp"
#include "Dialogs/Message.hpp"

void
MessageOperationEnvironment::SetErrorMessage(const char *text) noexcept
{
  ShowMessageBox(text, _T(""), MB_OK|MB_ICONERROR);
}
