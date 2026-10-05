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
  plan_tests(10);

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

  return exit_status();
}
