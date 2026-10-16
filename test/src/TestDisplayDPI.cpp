// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Hardware/DisplayDPI.hpp"
#include "TestUtil.hpp"

static void
CheckUnchanged(PixelSize size, UnsignedPoint2D dpi) noexcept
{
  const auto out = Display::AlignDpiToPixelAxes(size, dpi);
  ok1(out.x == dpi.x && out.y == dpi.y);
}

int
main()
{
  plan_tests(19);

  /* HiBreak: 824x1648 portrait pixels with landscape inches
     (xdpi=188, ydpi=667).  Pair the short physical edge with the
     short pixel edge. */
  {
    const auto out = Display::AlignDpiToPixelAxes({824, 1648}, {188, 667});
    ok1(out.x == 824u * 667u / 1648u);
    ok1(out.y == 1648u * 188u / 824u);
  }

  /* Same panel already rotated to landscape, axes still swapped. */
  {
    const auto out = Display::AlignDpiToPixelAxes({1648, 824}, {667, 188});
    ok1(out.x == 1648u * 188u / 824u);
    ok1(out.y == 824u * 667u / 1648u);
  }

  CheckUnchanged({824, 1648}, {334, 376});
  CheckUnchanged({1920, 1080}, {400, 400});
  CheckUnchanged({1080, 1920}, {400, 400});

  /* Square buffer: no pixel orientation to pair with. */
  CheckUnchanged({480, 480}, {96, 200});

  CheckUnchanged({0, 1648}, {188, 667});
  CheckUnchanged({824, 1648}, {0, 667});

  /* HiBreak Pro: millimetres are junk; Android density 300 is the
     6.13" 300 PPI panel. */
  {
    const auto out = Display::SanitizeDisplayDpi({824, 1648},
                                                 {188, 667}, 300);
    ok1(out.x == 300);
    ok1(out.y == 300);
  }

  /* Swapped axes but plausible; density matches after align. */
  {
    const auto out = Display::SanitizeDisplayDpi({800, 1280},
                                                 {200, 512}, 320);
    ok1(out.x == 320);
    ok1(out.y == 320);
  }

  /* Physical DPI within 20% of the density bucket: keep physical. */
  {
    const auto out = Display::SanitizeDisplayDpi({1080, 1920},
                                                 {440, 440}, 480);
    ok1(out.x == 440 && out.y == 440);
  }

  /* Physical DPI far from density: use density. */
  {
    const auto out = Display::SanitizeDisplayDpi({1080, 1920},
                                                 {200, 200}, 400);
    ok1(out.x == 400);
    ok1(out.y == 400);
  }

  /* No density: geometric align only. */
  {
    const auto out = Display::SanitizeDisplayDpi({824, 1648},
                                                 {188, 667}, 0);
    ok1(out.x == 824u * 667u / 1648u);
    ok1(out.y == 1648u * 188u / 824u);
  }

  return exit_status();
}
