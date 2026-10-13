// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/InfoBoxLayout.hpp"
#include "TestUtil.hpp"

static void
ExpectSize(unsigned screen_width, unsigned screen_height,
           unsigned width, unsigned height)
{
  const auto layout =
    InfoBoxLayout::Calculate(PixelRect{PixelSize{screen_width, screen_height}},
                             InfoBoxSettings::Geometry::RIGHT_5);
  ok1(layout.control_size.width == width);
  ok1(layout.control_size.height == height);
}

int
main()
{
  plan_tests(8);

  ExpectSize(1000, 1000, 200, 200);
  ExpectSize(500, 1000, 100, 200);
  ExpectSize(2000, 1000, 280, 200);
  ExpectSize(1400, 1000, 245, 200);

  return exit_status();
}
