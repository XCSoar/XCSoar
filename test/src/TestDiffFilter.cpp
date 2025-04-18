// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Math/DiffFilter.hpp"
#include "TestUtil.hpp"

#include <cstdio>

int
main()
{
  plan_tests(192 + 232 + 40);

  DiffFilter df(0);

  // Test steady-state response scaling
  for (long dY = 1; dY <= 10000000; dY *= 10) {
    df.Reset();
    for (int Y = dY; Y < 30 * dY; Y += dY) {
      if (Y < 6 * dY)
        // Give the filter time to calm down before testing for steady state
        df.Update(Y);
      else {
        // test if the filter response is close enough to dX
        //
        // the discrete filter design results in steady-state error
        // of approximately 3.4507 percent
        auto error = fabs((df.Update(Y) - dY) / dY);
        ok1(error < 0.035);
      }
    }
  }

  // Test steady-state response scaling with reset(0, dX) call
  for (long dY = 1; dY <= 10000000; dY *= 10) {
    df.Reset(0, dY);
    for (int Y = dY; Y < 30 * dY; Y += dY) {
      // test if the filter response is close enough to dX
      //
      // the discrete filter design results in steady-state error
      // of approximately 3.4507 percent
      auto error = fabs((df.Update(Y) - dY) / dY);
      ok1(error < 0.035);
    }
  }

  df.Reset();
  auto p = M_2PI / 10;
  for (int X = 0; X < 50; X += 1) {
    // Y = sin(2 * pi * (X/10)
    auto Y = sin(p * X);

    auto dY_shifted = cos(p * (X - 3)) * p;
    auto dY_filter = df.Update(Y);
    auto error = fabs(dY_filter - dY_shifted);
    if (X >= 10)
      ok1(error < 0.05);
  }

  return exit_status();
}
