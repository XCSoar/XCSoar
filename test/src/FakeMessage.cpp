// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Message.hpp"

#include <stdio.h>

void
Message::AddMessage(const TCHAR *text,
                    [[maybe_unused]] const TCHAR *data) noexcept
{
  _ftprintf(stderr, _T("%s\n"), text);
}
