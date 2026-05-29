// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioLook.hpp"
#include "Colors.hpp"
#include "Asset.hpp"
#include "AutoFont.hpp"
#include "FontDescription.hpp"
#include "Resources.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Features.hpp" // for HAVE_TEXT_CACHE

#ifdef HAVE_TEXT_CACHE
#include "ui/canvas/custom/Cache.hpp"
#endif

#include <algorithm>

namespace {

static constexpr const char *const VARIO_LABEL_PROBE = "Gross";
static constexpr const char *const VARIO_VALUE_PROBE = "+99.9";

struct VarioRowFontHeights {
  unsigned value_height;
  unsigned label_height;
};

/** Value font fills the row slot; label font is smaller in a narrow column. */
[[gnu::pure]]
static VarioRowFontHeights
ComputeVarioRowFontHeights(unsigned text_width, unsigned row_slot,
                           unsigned scale_title_font) noexcept
{
  const unsigned label_column_w = std::max(1u,
    text_width * VarioLook::INNER_ROW_LABEL_WIDTH_PERCENT / 100u);
  const unsigned value_column_w =
    std::max(1u, text_width - label_column_w);
  const unsigned value_budget = std::max(1u,
    value_column_w * VarioLook::INNER_ROW_FONT_PERCENT / 100u);
  const unsigned label_budget = std::max(1u,
    label_column_w * VarioLook::INNER_ROW_FONT_PERCENT / 100u);
  const unsigned clamped_scale = std::clamp(scale_title_font, 50u, 150u);
  const unsigned desired_value = std::max(6u, row_slot * clamped_scale / 100u);

  unsigned value_height = std::min(row_slot, desired_value);
  Font value_font;
  while (value_height > 6) {
    value_font.Load(FontDescription(value_height, true));
    if (value_font.TextSize(VARIO_VALUE_PROBE).width <= value_budget)
      break;
    --value_height;
  }

  const unsigned label_cap = std::max(6u,
    value_height * VarioLook::INNER_LABEL_FONT_PERCENT / 100u);
  unsigned label_height = std::min(row_slot, label_cap);
  Font label_font;
  while (label_height > 6) {
    label_font.Load(FontDescription(label_height));
    if (label_font.TextSize(VARIO_LABEL_PROBE).width <= label_budget)
      break;
    --label_height;
  }
  label_height = std::min(label_height, value_height);

  return {value_height, label_height};
}

} // namespace

void
VarioLook::Initialise(bool _inverse, bool _colors,
                      unsigned width,
                      const Font &_text_font)
{
  inverse = _inverse;
  colors = _colors;
  text_font = &_text_font;

  if (inverse) {
    background_color = COLOR_BLACK;
    text_color = COLOR_WHITE;
    dimmed_text_color = Color(0xa0, 0xa0, 0xa0);
    sink_color = Color(0xc4, 0x80, 0x1e);
    lift_color = Color(0x1e, 0xf1, 0x73);
  } else {
    background_color = COLOR_WHITE;
    text_color = COLOR_BLACK;
    dimmed_text_color = Color((uint8_t)~0xa0, (uint8_t)~0xa0, (uint8_t)~0xa0);
    sink_color = Color(0xa3, 0x69, 0x0d);
    lift_color = Color(0x19, 0x94, 0x03);
  }

  background_brush.Create(background_color);
  sink_brush.Create(sink_color);
  lift_brush.Create(lift_color);

  arc_pen.Create(Layout::ScalePenWidth(2), text_color);
  tick_pen.Create(Layout::ScalePenWidth(1), dimmed_text_color);

  thick_background_pen.Create(Layout::Scale(5), background_color);
  thick_sink_pen.Create(Layout::Scale(5), sink_color);
  thick_lift_pen.Create(Layout::Scale(5), lift_color);

  scale_face_color = inverse ? COLOR_WHITE : COLOR_BLACK;
  scale_ink_color = inverse ? COLOR_BLACK : COLOR_WHITE;

  accent[0] = COLOR_GRAY;
  if (HasColors() && colors) {
    accent[1] = inverse ? COLOR_INVERSE_RED : COLOR_INFOBOX_RED;
    accent[2] = inverse ? COLOR_INFOBOX_BLUE_INVERSE : COLOR_INFOBOX_BLUE;
    accent[3] = inverse ? COLOR_INFOBOX_GREEN_INVERSE : COLOR_INFOBOX_GREEN;
    accent[4] = inverse ? COLOR_INVERSE_YELLOW : COLOR_YELLOW;
    accent[5] = inverse ? COLOR_INVERSE_MAGENTA : COLOR_MAGENTA;
  } else
    std::fill(accent + 1, accent + 6,
              inverse ? COLOR_WHITE : COLOR_BLACK);

  climb_bitmap.Load(inverse ? IDB_CLIMBSMALLINV : IDB_CLIMBSMALL);

  ReinitialiseLayout(width);
}

void
VarioLook::ReinitialiseLayout(unsigned width)
{
  ReinitialiseLayout(width, 100, std::max(1u, width / 2u), 32,
                      Layout::Scale(8));
}

void
VarioLook::ReinitialiseLayout(unsigned /*panel_width*/,
                              unsigned scale_title_font,
                              unsigned text_column_width,
                              unsigned max_row_height,
                              unsigned arc_label_width) noexcept
{
  ++layout_generation;

  const unsigned text_width = std::max(1u, text_column_width);
  const unsigned row_slot = std::max(1u, max_row_height);
  const VarioRowFontHeights heights =
    ComputeVarioRowFontHeights(text_width, row_slot, scale_title_font);

  label_font.Load(FontDescription(heights.label_height));
  value_font.Load(FontDescription(heights.value_height, true));

  FontDescription small_value_font_d(10);
  AutoSizeFont(small_value_font_d,
               std::max(6u, arc_label_width), "5");
  arc_label_font.Load(small_value_font_d);

  const unsigned unit_font_height =
    std::max(heights.value_height * 2u / 5u, 7u);
  unit_font.Load(FontDescription(unit_font_height));
  unit_fraction_pen.Create(Layout::ScaleFinePenWidth(1), text_color);

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif
}
