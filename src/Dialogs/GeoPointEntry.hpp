// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class CoordinateFormat : uint8_t;
struct GeoPoint;

bool
GeoPointEntryDialog(const char *caption, GeoPoint &value,
                    CoordinateFormat format,
                    bool nullable=false);
