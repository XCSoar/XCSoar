// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;
struct BrokenDateTime;

void
MarkLocation(const GeoPoint &loc, const BrokenDateTime &time);
