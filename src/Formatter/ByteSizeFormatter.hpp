// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <cstdint>

void FormatByteSize(char *buffer, size_t size,
                    uint64_t bytes, bool simple = false);
