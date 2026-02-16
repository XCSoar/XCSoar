// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string_view>

typedef void (*pt2Event)(const char *);

namespace InputEvents {

[[gnu::pure]]
int findGCE(const char *data);

[[gnu::pure]]
int findNE(const char *data);

[[gnu::pure]]
pt2Event
findEvent(std::string_view name) noexcept;

} // namespace InputEvents
