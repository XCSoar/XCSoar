// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Screen/Layout.hpp"
#include "TestUtil.hpp"
#include "util/Macros.hpp"

#include <algorithm>

/**
 * Independent copy of the scale math in Screen/Layout.cpp, so a
 * formula change in Initialise() fails this test.
 */
struct Expected {
  bool landscape;
  bool small_screen;
  unsigned min_screen_pixels;
  unsigned scale_1024;
  unsigned scale;
  unsigned vdpi;
  unsigned pen_width_scale;
  unsigned fine_pen_width_scale;
  unsigned pt_scale;
  unsigned vpt_scale;
  unsigned font_scale;
  unsigned text_padding;
  unsigned minimum_control_height;
  unsigned maximum_control_height;
  unsigned hit_radius;

  static unsigned
  Shift10(unsigned value, unsigned factor) noexcept
  {
    return (value * factor) >> 10;
  }

  static unsigned
  SmallScreenAdjust(unsigned value, bool small) noexcept
  {
    return small ? value * 2 / 3 : value;
  }

  static bool
  IsSmallScreen(PixelSize size, UnsignedPoint2D dpi) noexcept
  {
    return size.width < size.height
      ? size.width < dpi.x * 5
      : size.height < dpi.y * 5;
  }

  static Expected
  Make(PixelSize size, UnsignedPoint2D dpi,
       unsigned ui_scale, bool has_touch) noexcept
  {
    Expected e{};
    e.landscape = size.width > size.height;
    e.min_screen_pixels = std::min(size.width, size.height);
    e.small_screen = IsSmallScreen(size, dpi);

    const bool square = size.width == size.height;
    e.scale_1024 = std::max(1024U, e.min_screen_pixels * 1024 /
                            (square ? 320 : 240));
    e.scale = e.scale_1024 / 1024;

    e.vdpi = SmallScreenAdjust(dpi.y, e.small_screen);
    e.pen_width_scale = std::max(1024u, dpi.x * 1024u / 80u);
    e.fine_pen_width_scale = std::max(1024u, dpi.x * 1024u / 160u);
    e.pt_scale = 1024 * dpi.y / 72;
    e.vpt_scale = SmallScreenAdjust(e.pt_scale, e.small_screen);
    e.font_scale = SmallScreenAdjust(1024 * dpi.y * ui_scale / 72 / 100,
                                    e.small_screen);

    e.text_padding = Shift10(2, e.vpt_scale);

    const unsigned font_h = Shift10(23, e.font_scale);
    e.minimum_control_height = std::min(font_h,
                                       e.min_screen_pixels / 12);

    if (has_touch) {
      e.maximum_control_height = Shift10(30, e.pt_scale);
      if (e.maximum_control_height < e.minimum_control_height)
        e.maximum_control_height = e.minimum_control_height;
    } else {
      e.maximum_control_height = e.minimum_control_height;
    }

    e.hit_radius = Shift10(has_touch ? 28 : 6, e.pt_scale);
    return e;
  }
};

static constexpr unsigned LAYOUT_CHECKS = 24;

static void
CheckLayout(const Expected &e) noexcept
{
  ok1(Layout::landscape == e.landscape);
  ok1(Layout::small_screen == e.small_screen);
  ok1(Layout::min_screen_pixels == e.min_screen_pixels);
  ok1(Layout::scale_1024 == e.scale_1024);
  ok1(Layout::scale == e.scale);
  ok1(Layout::vdpi == e.vdpi);
  ok1(Layout::pen_width_scale == e.pen_width_scale);
  ok1(Layout::fine_pen_width_scale == e.fine_pen_width_scale);
  ok1(Layout::pt_scale == e.pt_scale);
  ok1(Layout::vpt_scale == e.vpt_scale);
  ok1(Layout::font_scale == e.font_scale);
  ok1(Layout::text_padding == e.text_padding);
  ok1(Layout::minimum_control_height == e.minimum_control_height);
  ok1(Layout::maximum_control_height == e.maximum_control_height);
  ok1(Layout::hit_radius == e.hit_radius);

  ok1(Layout::Scale(240u) == Expected::Shift10(240, e.scale_1024));
  ok1(Layout::PtScale(12) == Expected::Shift10(12, e.pt_scale));
  ok1(Layout::VptScale(12) == Expected::Shift10(12, e.vpt_scale));
  ok1(Layout::FontScale(23) == Expected::Shift10(23, e.font_scale));
  ok1(Layout::GetInflightButtonHeight() ==
      Expected::Shift10(Layout::inflight_button_pt, e.pt_scale));
  ok1(Layout::ScalePenWidth(1) == Expected::Shift10(1, e.pen_width_scale));
  ok1(Layout::ScaleFinePenWidth(1) ==
      Expected::Shift10(1, e.fine_pen_width_scale));
  ok1(Layout::FastScale(10u) == 10u * e.scale);

  const PixelSize scaled = Layout::Scale(PixelSize{10, 20});
  ok1(scaled.width == Expected::Shift10(10, e.scale_1024) &&
      scaled.height == Expected::Shift10(20, e.scale_1024));
}

static void
TestCase(PixelSize size, UnsignedPoint2D dpi,
         unsigned ui_scale, bool has_touch) noexcept
{
  Layout::Initialise(size, dpi, ui_scale, has_touch);
  CheckLayout(Expected::Make(size, dpi, ui_scale, has_touch));
}

/**
 * Literal goldens for square vs landscape.  Square is never
 * landscape, uses divisor 320, and small-screen on a square uses
 * y DPI.  Landscape is width>height and uses the short-edge (y)
 * DPI; flipping 480x640 to 640x480 keeps the same scale_1024.
 */
static constexpr unsigned SQUARE_LANDSCAPE_CHECKS = 25;

static void
TestSquareAndLandscape() noexcept
{
  const UnsignedPoint2D dpi96{96, 96};

  Layout::Initialise({240, 320}, dpi96, 100, false);
  ok1(!Layout::landscape);
  ok1(Layout::scale_1024 == 1024);
  ok1(Layout::Scale(240u) == 240);

  Layout::Initialise({320, 240}, dpi96, 100, false);
  ok1(Layout::landscape);
  ok1(Layout::scale_1024 == 1024);
  ok1(Layout::Scale(240u) == 240);
  ok1(Layout::min_screen_pixels == 240);

  Layout::Initialise({320, 320}, dpi96, 100, false);
  ok1(!Layout::landscape);
  ok1(Layout::scale_1024 == 1024);

  Layout::Initialise({480, 480}, dpi96, 100, false);
  ok1(!Layout::landscape);
  ok1(Layout::scale_1024 == 480 * 1024 / 320);

  Layout::Initialise({480, 640}, dpi96, 100, false);
  ok1(!Layout::landscape);
  ok1(Layout::scale_1024 == 480 * 1024 / 240);

  Layout::Initialise({640, 480}, dpi96, 100, false);
  ok1(Layout::landscape);
  ok1(Layout::scale_1024 == 480 * 1024 / 240);

  /* 1px wider than square still uses 240, not 320 */
  Layout::Initialise({481, 480}, dpi96, 100, false);
  ok1(Layout::landscape);
  ok1(Layout::scale_1024 == 480 * 1024 / 240);

  /* square small-screen uses y_dpi (width < height is false) */
  Layout::Initialise({480, 480}, {96, 200}, 100, false);
  ok1(!Layout::landscape);
  ok1(Layout::small_screen);

  Layout::Initialise({480, 480}, {200, 96}, 100, false);
  ok1(!Layout::landscape);
  ok1(!Layout::small_screen);

  /* landscape small-screen uses y_dpi (the short edge) */
  Layout::Initialise({800, 480}, {96, 200}, 100, false);
  ok1(Layout::landscape);
  ok1(Layout::small_screen);

  Layout::Initialise({800, 480}, {200, 96}, 100, false);
  ok1(Layout::landscape);
  ok1(!Layout::small_screen);
}

int
main()
{
  static constexpr UnsignedPoint2D dpi96{96, 96};
  static constexpr UnsignedPoint2D dpi400{400, 400};

  static constexpr struct {
    PixelSize size;
    UnsignedPoint2D dpi;
    unsigned ui_scale;
    bool has_touch;
  } cases[] = {
    /* 240x320 @ 96 DPI: historical base, 2.5" short edge, small */
    {{240, 320}, dpi96, 100, false},
    {{320, 240}, dpi96, 100, false},
    {{320, 320}, dpi96, 100, false},
    {{480, 480}, dpi96, 100, false},
    {{480, 640}, dpi96, 100, false},

    /* 5 inch is size < dpi*5, not <= */
    {{479, 800}, dpi96, 100, false},
    {{480, 800}, dpi96, 100, false},

    {{1920, 1080}, dpi96, 100, false},
    {{1920, 1080}, dpi96, 200, false},
    {{1920, 1080}, dpi96, 100, true},
    /* high ui_scale + touch: PtScale(30) can be below minimum */
    {{1920, 1080}, dpi96, 400, true},

    {{1080, 1920}, dpi400, 100, true},
    {{1080, 1920}, {96, 400}, 100, false},
  };

  plan_tests(1 + ARRAY_SIZE(cases) * LAYOUT_CHECKS +
             SQUARE_LANDSCAPE_CHECKS);

  ok1(Layout::ScaleSupported());

  for (const auto &c : cases)
    TestCase(c.size, c.dpi, c.ui_scale, c.has_touch);

  TestSquareAndLandscape();

  return exit_status();
}
