// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Point.hpp"

#include <vector>

class GeoBounds;

class TracePointVector : public std::vector<TracePoint> {
public:
  void ScanBounds(GeoBounds &bounds) const noexcept;
};

/**
 * A direct-access storage for TracePoint pointers that refer to
 * elements of a Trace.  Be careful: all pointers get Invalidated when
 * the Trace gets thinned.  Be sure to check Trace::GetModifySerial()
 * for updates.
 */
class TracePointerVector : public std::vector<const TracePoint *> {
};
