// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <cstddef>
#include <cstdint>

void FormatByteSize(TCHAR *buffer, size_t size,
                    uint64_t bytes, bool simple = false);
