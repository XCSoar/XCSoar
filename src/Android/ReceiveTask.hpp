// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>
#include <string_view>

class OrderedTask;

[[gnu::pure]]
bool
HasReceivedTask() noexcept;

std::unique_ptr<OrderedTask>
GetReceivedTask() noexcept;

void
ReceiveXCTrackTask(std::string_view data);
