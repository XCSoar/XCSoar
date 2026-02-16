// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>

void FormatByteSize(char *buffer, size_t size,
                    unsigned long bytes, bool simple = false);
