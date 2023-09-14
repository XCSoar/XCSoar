// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct AGeoPoint;

class AbortIntersectionTest {
public:
  [[gnu::pure]]
  virtual bool Intersects(const AGeoPoint &destination) const noexcept = 0;
};
