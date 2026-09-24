// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;

bool
HasSystemTools() noexcept;

void
ShowPendingSystemToolResult() noexcept;

std::unique_ptr<Widget>
CreateSystemToolsPanel();
