// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <cstdint>

class RadioFrequency;

bool
RadioFrequencyEntryDialog(const TCHAR *caption, RadioFrequency &value,
                    bool nullable=false);
