// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct GeoPoint;

[[gnu::pure]]
bool
IsPanning();

void
EnterPan();

bool
PanTo(const GeoPoint &location);

/**
 * Low-level version of LeavePan().  It disables panning in the map
 * and updates the input mode, but does not restore the page layout.
 * Only to be used by the pages library.
 */
void
DisablePan();

void
LeavePan();

void
TogglePan();
