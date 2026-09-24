// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/display/wayland/Scale.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(8 + 6 + 5 + 10 + 6 + 3);

  ok1(Wayland::ToPhysicalPixels(640, Wayland::SCALE_100) == 640);
  ok1(Wayland::ToPhysicalPixels(640, Wayland::SCALE_200) == 1280);
  ok1(Wayland::ToPhysicalPixels(640, Wayland::SCALE_125) == 800);
  ok1(Wayland::ToPhysicalPixels(640, Wayland::SCALE_150) == 960);
  ok1(Wayland::ToPhysicalPixels(0, Wayland::SCALE_200) == 0);
  ok1(Wayland::ToPhysicalPixels(1, Wayland::SCALE_125) == 1);
  ok1(Wayland::ToPhysicalPixels(1, Wayland::SCALE_150) == 2);
  ok1(Wayland::ToPhysicalPixels(708, Wayland::SCALE_200) == 1416);

  ok1(Wayland::ToIntegerScale(Wayland::SCALE_100) == 1);
  ok1(Wayland::ToIntegerScale(Wayland::SCALE_200) == 2);
  ok1(Wayland::ToIntegerScale(Wayland::SCALE_125) == 1);
  ok1(Wayland::ToIntegerScale(Wayland::SCALE_150) == 2);
  ok1(Wayland::ToIntegerScale(0) == 1);
  ok1(Wayland::ToIntegerScale(59) == 1);

  /* 2880×1800 mode, logical 1440×900 → 200% */
  ok1(Wayland::ToScale120ths(2880, 1440) == Wayland::SCALE_200);
  ok1(Wayland::ToScale120ths(1920, 1920) == Wayland::SCALE_100);
  ok1(Wayland::ToScale120ths(1920, 0) == Wayland::SCALE_100);
  ok1(Wayland::ToScale120ths(1920, 1280) == Wayland::SCALE_150);
  ok1(Wayland::ToScale120ths(0, 1440) == 0);

  const PixelSize window{640, 480};

  const auto fractional_200 =
    Wayland::ChooseBuffer(window, Wayland::SCALE_200, true);
  ok1(fractional_200.size == PixelSize(1280, 960));
  ok1(fractional_200.buffer_scale == 1);

  const auto fractional_125 =
    Wayland::ChooseBuffer(window, Wayland::SCALE_125, true);
  ok1(fractional_125.size == PixelSize(800, 600));
  ok1(fractional_125.buffer_scale == 1);

  const auto integer_200 =
    Wayland::ChooseBuffer(window, Wayland::SCALE_200, false);
  ok1(integer_200.size == PixelSize(1280, 960));
  ok1(integer_200.buffer_scale == 2);

  /* 125% without a viewport cannot be 1.25×, so it stays 1× */
  const auto integer_125 =
    Wayland::ChooseBuffer(window, Wayland::SCALE_125, false);
  ok1(integer_125.size == PixelSize(640, 480));
  ok1(integer_125.buffer_scale == 1);

  const auto integer_150 =
    Wayland::ChooseBuffer(window, Wayland::SCALE_150, false);
  ok1(integer_150.size == PixelSize(1280, 960));
  ok1(integer_150.buffer_scale == 2);

  ok1(Wayland::ToLogicalPixels(1280, Wayland::SCALE_200) == 640);
  ok1(Wayland::ToLogicalPixels(800, Wayland::SCALE_125) == 640);
  ok1(Wayland::ToLogicalPixels(640, Wayland::SCALE_100) == 640);
  ok1(Wayland::ToPhysicalCoord(640, Wayland::SCALE_200) == 1280);
  ok1(Wayland::ToPhysicalCoord(-10, Wayland::SCALE_200) == -20);
  ok1(Wayland::ToPhysicalCoord(0, Wayland::SCALE_200) == 0);
  ok1(Wayland::FromIntegerScale(2) == Wayland::SCALE_200);
  ok1(Wayland::FromIntegerScale(0) == Wayland::SCALE_100);
  ok1(Wayland::ToPhysicalPoint({640, 480}, Wayland::SCALE_200) ==
      PixelPoint(1280, 960));

  return exit_status();
}
