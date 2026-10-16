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

static void
TestVarioAnchor()
{
  /* A status bar or cutout puts the InfoBox area below y = 0. The
     three-high gauge must start there, level with the boxes under it. */
  const PixelRect area{5, 40, 1605, 1040};
  const auto right9 =
    InfoBoxLayout::Calculate(area, InfoBoxSettings::Geometry::RIGHT_9_VARIO);
  ok1(right9.vario.top == area.top);
  ok1(right9.vario.bottom ==
      area.top + int(right9.control_size.height * 3));
  ok1(right9.positions[6].top == right9.vario.bottom);

  const auto left6 =
    InfoBoxLayout::Calculate(area,
                             InfoBoxSettings::Geometry::LEFT_6_RIGHT_3_VARIO);
  ok1(left6.vario.top == area.top);
  ok1(left6.positions[6].top == left6.vario.bottom);

  const PixelRect portrait{0, 30, 600, 1230};
  const auto bottom =
    InfoBoxLayout::Calculate(portrait,
                             InfoBoxSettings::Geometry::BOTTOM_8_VARIO);
  ok1(bottom.vario.bottom == portrait.bottom);
  ok1(bottom.vario.GetHeight() == bottom.control_size.height * 2);
  ok1(bottom.vario.GetWidth() == bottom.control_size.width);
}

int
main()
{
  plan_tests(8 + 8);

  ExpectSize(1000, 1000, 200, 200);
  ExpectSize(500, 1000, 100, 200);
  ExpectSize(2000, 1000, 280, 200);
  ExpectSize(1400, 1000, 245, 200);
  TestVarioAnchor();

  return exit_status();
}
