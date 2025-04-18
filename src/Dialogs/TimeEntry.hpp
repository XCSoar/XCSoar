// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class RoughTime;
class RoughTimeDelta;

bool
TimeEntryDialog(const TCHAR *caption, RoughTime &value,
                RoughTimeDelta time_zone, bool nullable=false);
