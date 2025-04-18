// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

struct FlatGeoPoint;

class FlatTriangleFanVisitor {
public:
  virtual void VisitFan(FlatGeoPoint origin,
                        std::span<const FlatGeoPoint> fan) noexcept = 0;
};
