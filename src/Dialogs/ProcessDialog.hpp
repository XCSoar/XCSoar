// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <functional>

#include <tchar.h>

struct DialogLook;
namespace UI { class SingleWindow; }

int
RunProcessDialog(UI::SingleWindow &parent,
                 const DialogLook &dialog_look,
                 const char *caption,
                 const char *const*argv,
                 std::function<int(int)> on_exit={}) noexcept;
