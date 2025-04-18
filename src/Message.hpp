// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

namespace Message {

void
AddMessage(const TCHAR *text, const TCHAR *data=nullptr) noexcept;

} // namespace Message
