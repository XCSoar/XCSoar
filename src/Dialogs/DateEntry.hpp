// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct BrokenDate;

bool
DateEntryDialog(const TCHAR *caption, BrokenDate &value,
                bool nullable=false);
