// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <functional>
#include <tchar.h>

namespace ConfigPanel {
void BorrowExtraButton(unsigned i, const char *caption,
                       std::function<void()> callback) noexcept;
void ReturnExtraButton(unsigned i);
};
