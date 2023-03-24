// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TestMath.hpp"
#include "Math/Point2D.hpp"
#include "Math/Line2D.hpp"
#include "TestUtil.hpp"

void
TestLine2D()
{
  typedef Line2D<DoublePoint2D> DoubleLine2D;

  const DoubleLine2D a({0, 0}, {4, 0});
  ok1(a.GetSquaredDistance() == 16);
  ok1(a.GetMiddle() == DoublePoint2D(2, 0));
  ok1(a.CrossProduct() == 0);
  ok1(a.Contains({0, 0}));
  ok1(a.Contains({-1, 0}));
  ok1(a.Contains({100, 0}));
  ok1(!a.Contains({2, 1}));
  ok1(a.Interpolate(0) == DoublePoint2D(0, 0));
  ok1(a.Interpolate(0.25) == DoublePoint2D(1, 0));
  ok1(a.Interpolate(1) == DoublePoint2D(4, 0));
  ok1(a.Interpolate(2) == DoublePoint2D(8, 0));
  ok1(a.Interpolate(-1) == DoublePoint2D(-4, 0));
  ok1(a.ProjectedRatio({2, 0}) == 0.5);
  ok1(a.ProjectedRatio({3, 0}) == 0.75);
  ok1(a.ProjectedRatio({2, 1}) == 0.5);
  ok1(a.ProjectedRatio({3, 10}) == 0.75);
  ok1(a.ProjectedRatio({6, 10}) == 1.5);
  ok1(a.SquareDistanceTo({2, 0}) == 0);
  ok1(a.SquareDistanceTo({6, 10}) == 100);

  const DoubleLine2D b({0, 0}, {0, 4});
  ok1(b.GetSquaredDistance() == 16);
  ok1(b.GetMiddle() == DoublePoint2D(0, 2));
  ok1(b.CrossProduct() == 0);
  ok1(b.Contains({0, 0}));
  ok1(b.Contains({0, -1}));
  ok1(b.Contains({0, 100}));
  ok1(!b.Contains({1, 2}));
  ok1(b.ProjectedRatio({0, 2}) == 0.5);
  ok1(b.ProjectedRatio({0, 3}) == 0.75);
  ok1(b.ProjectedRatio({1, 2}) == 0.5);
  ok1(b.ProjectedRatio({10, 3}) == 0.75);
  ok1(b.ProjectedRatio({10, 6}) == 1.5);

  const DoubleLine2D c({1, 5}, {3, 1});
  ok1(c.GetSquaredDistance() == 20);
  ok1(c.GetMiddle() == DoublePoint2D(2, 3));
  ok1(c.CrossProduct() == -14);
  ok1(c.Contains({1, 5}));
  ok1(c.Contains({1.5, 4}));
  ok1(c.Contains({0, 7}));
  ok1(!c.Contains({0, 0}));
  ok1(c.ProjectedRatio({1, 5}) == 0);
  ok1(c.ProjectedRatio({2, 3}) == 0.5);
  ok1(c.ProjectedRatio({5, 2}) == 1);
  ok1(c.ProjectedRatio({0, 2}) == 0.5);
  ok1(c.Project({5, 2}) == DoublePoint2D(3, 1));
  ok1(c.Project({0, 2}) == DoublePoint2D(2, 3));
  ok1(c.SquareDistanceTo({1, 5}) == 0);
  ok1(c.SquareDistanceTo({5, 2}) == 5);
  ok1(c.SquareDistanceTo({0, 2}) == 5);
}
