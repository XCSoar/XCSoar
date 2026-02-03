// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Screen/Layout.hpp"
#include "ui/dim/Size.hpp"
#include "ui/display/Display.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "Asset.hpp"

#include <algorithm>

namespace Layout {

#if defined(USE_X11) || defined(MESA_KMS) || defined(USE_WAYLAND)
static constexpr unsigned
MMToDPI(unsigned pixels, unsigned mm) noexcept
{
  /* 1 inch = 25.4 mm */
  return pixels * 254 / (mm * 10);
}
#endif

bool landscape = false;
unsigned min_screen_pixels = 512;
unsigned scale = 1;
unsigned scale_1024 = 1024;
unsigned pen_width_scale = 1024;
unsigned fine_pen_width_scale = 1024;
unsigned vdpi = 72;
unsigned pt_scale = 1024;
unsigned vpt_scale = 1024;
unsigned font_scale = 1024;
unsigned text_padding = 2;
unsigned minimum_control_height = 20, maximum_control_height = 44;
unsigned hit_radius = 10;

/**
 * Is the given pixel size smaller than 5 inch?
 */
static constexpr bool
IsSmallScreen(unsigned size, unsigned dpi) noexcept
{
  return size < dpi * 5;
}

/**
 * Is the small edge smaller than 5 inch?
 */
static constexpr bool
IsSmallScreen(unsigned width, unsigned height,
              unsigned x_dpi, unsigned y_dpi) noexcept
{
  return width < height
    ? IsSmallScreen(width, x_dpi)
    : IsSmallScreen(height, y_dpi);
}

/**
 * Is the small edge smaller than 5 inch?
 */
static constexpr bool
IsSmallScreen(PixelSize size, UnsignedPoint2D dpi) noexcept
{
  return IsSmallScreen(size.width, size.height, dpi.x, dpi.y);
}

[[gnu::pure]]
static PixelSize
GetDisplaySize([[maybe_unused]] const UI::Display &display, [[maybe_unused]] PixelSize fallback) noexcept
{
#if defined(USE_X11) || defined(USE_GDI)
  return display.GetSize();
#else
  return fallback;
#endif
}

void
Initialise(const UI::Display &display, PixelSize new_size,
           unsigned ui_scale, unsigned custom_dpi) noexcept
{
  const unsigned width = new_size.width, height = new_size.height;

  min_screen_pixels = std::min(width, height);
  landscape = width > height;
  const bool square = width == height;

  if constexpr (!ScaleSupported())
    return;

  const auto dpi = Display::GetDPI(display, custom_dpi);
  
  /* Get physical DPI for font scaling to maintain same physical size
     regardless of forced DPI. */
  UnsignedPoint2D physical_dpi = dpi;
#if defined(USE_X11) || defined(MESA_KMS) || defined(USE_WAYLAND)
  {
    const auto size = display.GetSize();
    const auto size_mm = display.GetSizeMM();
    if (size.width > 0 && size.height > 0 &&
        size_mm.width > 0 && size_mm.height > 0) {
      physical_dpi = {
        MMToDPI(size.width, size_mm.width),
        MMToDPI(size.height, size_mm.height),
      };
    }
  }
#endif

  /* When compositor uses content scale (e.g. Wayland buffer scale), use
     effective_dpi = physical_dpi / content_scale so we do not double-scale
     fonts and pens. Apply ~1.2x so fonts and control heights match typical
     X11 (Xft.dpi often 120 when physical size unknown). */
  const unsigned content_scale = Display::GetContentScale(display);
  UnsignedPoint2D effective_dpi = physical_dpi;
  if (content_scale > 1) {
    effective_dpi.x = physical_dpi.x * 120 / (content_scale * 100);
    effective_dpi.y = physical_dpi.y * 120 / (content_scale * 100);
    if (effective_dpi.x < 1)
      effective_dpi.x = 1;
    if (effective_dpi.y < 1)
      effective_dpi.y = 1;
    if (effective_dpi.x > physical_dpi.x)
      effective_dpi.x = physical_dpi.x;
    if (effective_dpi.y > physical_dpi.y)
      effective_dpi.y = physical_dpi.y;
  }
  
  const bool is_small_screen = IsSmallScreen(GetDisplaySize(display, new_size),
                                             dpi);

  const auto SmallScreenAdjust = [is_small_screen](unsigned value) constexpr noexcept {
    if (is_small_screen)
      /* small screens (on portable devices) use a smaller font because
         the viewing distance is usually smaller */
      value =  value * 2 / 3;
    return value;
  };

  // always start w/ shortest dimension
  // square should be shrunk
  scale_1024 = std::max(1024U, min_screen_pixels * 1024 / (square ? 320 : 240));
  scale = scale_1024 / 1024;

  /* Use effective DPI for font/pen scaling (physical when content_scale is 1,
     physical/content_scale when compositor scales so we do not double-scale). */
  vdpi = SmallScreenAdjust(effective_dpi.y);

  pen_width_scale = std::max(1024u, effective_dpi.x * 1024u / 80u);
  fine_pen_width_scale = std::max(1024u, effective_dpi.x * 1024u / 160u);

  pt_scale = 1024 * effective_dpi.y / 72;

  vpt_scale = SmallScreenAdjust(pt_scale);

  font_scale = SmallScreenAdjust(1024 * effective_dpi.y * ui_scale / 72 / 100);

  text_padding = VptScale(2);

  minimum_control_height = std::min(FontScale(23),
                                    min_screen_pixels / 12);

  if (HasTouchScreen()) {
    /* larger rows for touch screens; use Scale() so height is in layout
       (logical) pixels. When content_scale > 1, divide by scale so physical
       height stays consistent; apply same 1.2 factor as effective_dpi. */
    unsigned touch_height = Scale(30u);
    if (content_scale > 1) {
      touch_height = touch_height / content_scale;
      touch_height = touch_height * 120 / 100;
    }
    maximum_control_height = touch_height;
    if (maximum_control_height < minimum_control_height)
      maximum_control_height = minimum_control_height;
  } else {
    maximum_control_height = minimum_control_height;
  }

  hit_radius = PtScale(HasTouchScreen() ? 28 : 6);
}

} // namespace Layout
