// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class Angle;

bool
NumberEntryDialog(const char *caption,
                  int &value, unsigned length);

bool
NumberEntryDialog(const char *caption,
                  unsigned &value, unsigned length);

bool AngleEntryDialog(const char *caption, Angle &value);
