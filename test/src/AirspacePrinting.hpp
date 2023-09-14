// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#define DO_PRINT

#include <iostream>

struct AirspaceAltitude;

std::ostream &
operator<<(std::ostream &os, const AirspaceAltitude &aa);
