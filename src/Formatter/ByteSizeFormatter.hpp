// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <cstddef>

void FormatByteSize(TCHAR *buffer, size_t size,
                    unsigned long bytes, bool simple = false);
