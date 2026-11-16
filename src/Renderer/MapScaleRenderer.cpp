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
#include "util/StaticString.hxx"
#include "util/StringBuffer.hxx"
#include "Formatter/UserUnits.hpp"

#include <cassert>

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
  icon.Draw(canvas,
            {slot_left + (int(slot) - int(scaled.width)) / 2,
             top + (int(height) - int(scaled.height)) / 2},
            icon_px);
}

static unsigned
MapLabelWidth(Canvas &canvas, const Font &font,
              const MapLabel &label) noexcept
{
  canvas.Select(font);

  const unsigned padding = Layout::GetTextPadding();
  const unsigned icon_px = font.GetHeight();
  const unsigned lead = IconSlot(label.icon, icon_px, padding);
  /* text sits one padding past an icon, and against the padding when
     there is none */
  const unsigned before_text = label.icon != nullptr ? lead + padding : lead;
  const unsigned trail = label.right_icon == nullptr
    ? 0
    : label.right_icon->GetScaledSize(icon_px).width + padding;
  return before_text + canvas.CalcTextSize(label.text).width + padding + trail;
}

static unsigned
MapLabelRowWidth(Canvas &canvas, const Font &font,
                 const MapLabelList &list) noexcept
{
  const unsigned gap = Layout::GetTextPadding() * 2;
  unsigned width = 0;

  for (const MapLabel &item : list) {
    if (width != 0)
      width += gap;
    width += MapLabelWidth(canvas, font, item);
  }

  return width;
}

PixelRect
DrawMapLabel(Canvas &canvas, const Font &font,
            int left, int bottom, const MapLabel &label) noexcept
{
  canvas.Select(font);

  const PixelSize text = canvas.CalcTextSize(label.text);
  const unsigned padding = Layout::GetTextPadding();
  const unsigned height = GetMapScaleBandHeight(font);
  const unsigned icon_px = font.GetHeight();
  const unsigned lead = IconSlot(label.icon, icon_px, padding);
  /* text sits one padding past an icon, and against the padding when
     there is none */
  const unsigned before_text = label.icon != nullptr ? lead + padding : lead;
  const unsigned trail = label.right_icon == nullptr
    ? 0
    : label.right_icon->GetScaledSize(icon_px).width + padding;

  PixelRect rc;
  rc.left = left;
  rc.bottom = bottom;
  rc.top = bottom - int(height);
  rc.right = left + int(before_text + text.width + padding + trail);

  /* white stays translucent; a warning or an error is opaque */
  const bool solid = label.fill != COLOR_WHITE;
  const Color ink = LabelInk(label.fill);
  DrawRoundedLabel(canvas, rc, 0xff, LabelFill(label.fill), solid);
  canvas.SetTextColor(ink);

  if (label.icon != nullptr)
    DrawSlotIcon(canvas, *label.icon, rc.left, lead,
                 rc.top, height, icon_px);
  if (label.right_icon != nullptr)
    DrawSlotIcon(canvas, *label.right_icon, rc.right - int(trail), trail,
                 rc.top, height, icon_px);

  canvas.SetBackgroundTransparent();
  canvas.DrawText({rc.left + int(before_text),
                   rc.top + (int(height) - int(text.height)) / 2},
                  label.text);
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
  if (MapLabelWidth(canvas, font, range_label) > unsigned(rc.GetWidth()))
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
    const unsigned used = MapLabelRowWidth(canvas, font, list);
    if (used + gap + MapLabelWidth(canvas, font, contour_label)
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
    const unsigned used = MapLabelRowWidth(canvas, font, list);
    const unsigned tail_width = MapLabelWidth(canvas, font, *tail);
    if (used + gap + tail_width <= unsigned(rc.GetWidth()) &&
        !list.checked_append(*tail))
      assert(false);
  }

  return DrawMapLabelRow(canvas, *look.overlay_font,
                        rc.left, rc.bottom - 1, list);
}
