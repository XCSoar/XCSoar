// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <tchar.h>

enum class Contest : uint8_t;

[[gnu::const]]
const char *
ContestToString(Contest contest) noexcept;
