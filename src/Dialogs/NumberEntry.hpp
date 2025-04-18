// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class Angle;

bool
NumberEntryDialog(const TCHAR *caption,
                  int &value, unsigned length);

bool
NumberEntryDialog(const TCHAR *caption,
                  unsigned &value, unsigned length);

bool AngleEntryDialog(const TCHAR *caption, Angle &value);
