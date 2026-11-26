// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapScaleRenderer.hpp"
#include "Renderer/TextInBox.hpp"
#include "Asset.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Icon.hpp"
#include "Screen/Layout.hpp"
#include "Projection/WindowProjection.hpp"
#include "Look/OverlayLook.hpp"
#include "Look/Colors.hpp"
#include "ui/canvas/TextWrapper.hpp"
#include "util/StaticString.hxx"
#include "util/StringBuffer.hxx"
#include "Formatter/UserUnits.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>

unsigned
GetMapScaleBandHeight(const Font &font) noexcept
{
  /* one padding, split above and below the text */
  return font.GetHeight() + Layout::GetTextPadding();
}

/** Black ink on a light fill, white ink on a dark one. */
static Color
LabelInk(Color fill) noexcept
{
#ifdef GREYSCALE
  /* memory canvas greyscale has no channels; mid grey is 128 */
  return fill.GetLuminosity() > 128 ? COLOR_BLACK : COLOR_WHITE;
#else
  const unsigned sum = unsigned(fill.Red()) + fill.Green() + fill.Blue();
  return sum > 384 ? COLOR_BLACK : COLOR_WHITE;
#endif
}

/**
 * On a monochrome display a light fill becomes white and a dark fill
 * black, so any caller colour still reads.
 */
static Color
LabelFill(Color fill) noexcept
{
  if (HasColors() && !IsDithered())
    return fill;

  return LabelInk(fill) == COLOR_BLACK ? COLOR_WHITE : COLOR_BLACK;
}

/** Icon width plus one padding, or just the padding when there is no icon. */
static unsigned
IconSlot(const MaskedIcon *icon, unsigned icon_px,
         unsigned padding) noexcept
{
  return icon == nullptr
    ? padding
    : icon->GetScaledSize(icon_px).width + padding;
}

static void
DrawSlotIcon(Canvas &canvas, const MaskedIcon &icon,
             int slot_left, unsigned slot,
             int top, unsigned height, unsigned icon_px) noexcept
{
  const PixelSize scaled = icon.GetScaledSize(icon_px);
  /* Draw() subtracts the hotspot; map symbols are centred, overlay
     icons are top-left.  Pass the visual top-left plus that hotspot. */
  int x = slot_left + (int(slot) - int(scaled.width)) / 2;
  int y = top + (int(height) - int(scaled.height)) / 2;
  const unsigned native_h = icon.GetSize().height;
  if (native_h > 0 && icon_px > 0) {
    x += int(icon.GetOrigin().x * icon_px / native_h);
    y += int(icon.GetOrigin().y * icon_px / native_h);
  }
  icon.Draw(canvas, {x, y}, icon_px);
}

struct LabelChrome {
  unsigned padding, before_text, trail, lead, icon_px;

  unsigned Chrome() const noexcept {
    return before_text + padding + trail;
  }
};

static LabelChrome
MakeChrome(const Font &font, const MapLabel &label) noexcept
{
  LabelChrome c;
  c.padding = Layout::GetTextPadding();
  c.icon_px = font.GetHeight();
  c.lead = IconSlot(label.icon, c.icon_px, c.padding);
  /* text sits one padding past an icon, and against the padding when
     there is none */
  c.before_text = label.icon != nullptr ? c.lead + c.padding : c.lead;
  c.trail = label.right_icon == nullptr
    ? 0
    : label.right_icon->GetScaledSize(c.icon_px).width + c.padding;
  return c;
}

struct LabelLayout {
  LabelChrome chrome;
  WrappedText wrapped;
  PixelSize size;
};

static LabelLayout
LayoutLabel(const Font &font, const MapLabel &label,
            unsigned max_width) noexcept
{
  LabelLayout l;
  l.chrome = MakeChrome(font, label);
  const unsigned chrome = l.chrome.Chrome();
  const unsigned text_w = font.TextSize(label.text).width;
  if (max_width == 0 || chrome + text_w <= max_width)
    l.wrapped = {};
  else {
    const unsigned inner = max_width > chrome ? max_width - chrome : 1;
    l.wrapped = WrapText(font, inner, label.text);
  }

  const unsigned n = l.wrapped.lines.empty()
    ? 1u
    : unsigned(l.wrapped.lines.size());
  l.size = {
    l.wrapped.lines.empty() ? chrome + text_w : max_width,
    n * font.GetHeight() + l.chrome.padding,
  };
  return l;
}

PixelSize
MapLabelSize(const Font &font, const MapLabel &label,
             unsigned max_width) noexcept
{
  return LayoutLabel(font, label, max_width).size;
}

static unsigned
MapLabelRowWidth(const Font &font, const MapLabelList &list) noexcept
{
  const unsigned gap = Layout::GetTextPadding() * 2;
  unsigned width = 0;

  for (const MapLabel &item : list) {
    if (width != 0)
      width += gap;
    width += MapLabelSize(font, item).width;
  }

  return width;
}

PixelRect
DrawMapLabel(Canvas &canvas, const Font &font,
            int left, int bottom, const MapLabel &label,
            unsigned max_width) noexcept
{
  canvas.Select(font);

  const auto layout = LayoutLabel(font, label, max_width);
  const auto &c = layout.chrome;
  const auto &size = layout.size;

  PixelRect rc;
  rc.left = left;
  rc.bottom = bottom;
  rc.top = bottom - int(size.height);
  rc.right = left + int(size.width);

  /* white stays translucent; a warning or an error is opaque */
  const bool solid = label.fill != COLOR_WHITE;
  const Color ink = LabelInk(label.fill);
  DrawRoundedLabel(canvas, rc, 0xff, LabelFill(label.fill), solid);

  if (label.progress_range > 0 && HasColors() && !IsDithered()) {
    const unsigned bar_w = rc.GetWidth();
    const unsigned pos = std::min(label.progress_position,
                                  label.progress_range);
    const unsigned filled =
      (unsigned)((uint64_t(pos) * bar_w) / label.progress_range);
    if (filled > 0) {
      PixelRect fill_rc = rc;
      fill_rc.right = fill_rc.left + int(filled);
      DrawRoundedLabel(canvas, fill_rc, 0xff, COLOR_XCSOAR_LIGHT, true);
    }
  }

  canvas.SetTextColor(ink);

  if (label.icon != nullptr)
    DrawSlotIcon(canvas, *label.icon, rc.left, c.lead,
                 rc.top, size.height, c.icon_px);
  if (label.right_icon != nullptr)
    DrawSlotIcon(canvas, *label.right_icon, rc.right - int(c.trail), c.trail,
                 rc.top, size.height, c.icon_px);

  canvas.SetBackgroundTransparent();

  const int text_x = rc.left + int(c.before_text);
  if (layout.wrapped.lines.empty()) {
    const PixelSize text = canvas.CalcTextSize(label.text);
    canvas.DrawText({text_x,
                     rc.top + (int(size.height) - int(text.height)) / 2},
                    label.text);
  } else {
    const unsigned line_h = font.GetHeight();
    int y = rc.top + int(c.padding) / 2;
    for (const auto &line : layout.wrapped.lines) {
      canvas.DrawText({text_x, y}, line.GetText(label.text));
      y += int(line_h);
    }
  }
  return rc;
}

int
DrawMapLabelRow(Canvas &canvas, const Font &font,
               int left, int bottom,
               const MapLabelList &list) noexcept
{
  const int gap = int(Layout::GetTextPadding()) * 2;
  int right = left;

  for (const MapLabel &item : list) {
    right = DrawMapLabel(canvas, font, left, bottom, item).right;
    left = right + gap;
  }

  return right;
}

static bool
AppendMapScale(MapLabelList &list,
               StaticString<80> &range,
               BasicStringBuffer<char, 32> &contour,
               Canvas &canvas,
               const WindowProjection &projection,
               const PixelRect &rc,
               const OverlayLook &look,
               unsigned contour_spacing_m) noexcept
{
  if (!projection.IsValid())
    return false;

  const Font &font = *look.overlay_font;
  canvas.Select(font);
  FormatUserMapScale(projection.GetScreenWidthMeters(),
                     range.buffer(), true);

  const MapLabel range_label{
    range.c_str(),
    &look.map_scale_left_icon,
    COLOR_WHITE,
    &look.map_scale_right_icon,
  };
  /* the whole scale label, icons included, not only the digits */
  if (MapLabelSize(font, range_label).width > unsigned(rc.GetWidth()))
    return false;

  if (!list.checked_append(range_label)) {
    assert(false);
    return false;
  }

  if (contour_spacing_m != 0) {
    FormatUserAltitude(double(contour_spacing_m), contour.data());
    const MapLabel contour_label{
      contour.c_str(),
      &look.contour_spacing_icon,
    };
    const unsigned gap = Layout::GetTextPadding() * 2;
    const unsigned used = MapLabelRowWidth(font, list);
    if (used + gap + MapLabelSize(font, contour_label).width
        <= unsigned(rc.GetWidth())) {
      if (!list.checked_append(contour_label)) {
        assert(false);
        return false;
      }
    }
  }

  return true;
}

unsigned
GetMapScaleAndTitleClearance(const Font &font) noexcept
{
  /* scale label, a gap, the title label, a gap for the condition stack */
  const unsigned band = GetMapScaleBandHeight(font);
  const unsigned gap = Layout::GetTextPadding();
  return band + 1 + gap + band + gap;
}

int
RenderMapScale(Canvas &canvas,
               const WindowProjection& projection,
               const PixelRect &rc,
               const OverlayLook &look,
               unsigned contour_spacing_m,
               const MapLabel *tail)
{
  StaticString<80> range;
  BasicStringBuffer<char, 32> contour;
  MapLabelList list;

  if (!AppendMapScale(list, range, contour, canvas, projection, rc,
                      look, contour_spacing_m))
    return rc.left;

  if (tail != nullptr) {
    const Font &font = *look.overlay_font;
    const unsigned gap = Layout::GetTextPadding() * 2;
    const unsigned used = MapLabelRowWidth(font, list);
    const unsigned tail_width = MapLabelSize(font, *tail).width;
    if (used + gap + tail_width <= unsigned(rc.GetWidth()) &&
        !list.checked_append(*tail))
      assert(false);
  }

  return DrawMapLabelRow(canvas, *look.overlay_font,
                        rc.left, rc.bottom - 1, list);
}
