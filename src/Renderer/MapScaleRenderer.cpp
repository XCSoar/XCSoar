// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapScaleRenderer.hpp"
#include "Renderer/TextInBox.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Screen/Layout.hpp"
#include "Projection/WindowProjection.hpp"
#include "Look/OverlayLook.hpp"
#include "util/StaticString.hxx"
#include "Formatter/UserUnits.hpp"

unsigned
GetMapScaleBandHeight(const Font &font) noexcept
{
  /* one padding, split above and below the text */
  return font.GetHeight() + Layout::GetTextPadding();
}

unsigned
GetMapScaleAndTitleClearance(const Font &font) noexcept
{
  /* scale pill, a gap, the title pill, a gap for the condition stack */
  const unsigned band = GetMapScaleBandHeight(font);
  const unsigned gap = Layout::GetTextPadding();
  return band + 1 + gap + band + gap;
}

int
RenderMapScale(Canvas &canvas,
               const WindowProjection& projection,
               const PixelRect &rc,
               const OverlayLook &look,
               unsigned contour_spacing_m)
{
  if (!projection.IsValid())
    return rc.left;

  StaticString<80> buffer;

  auto map_width = projection.GetScreenWidthMeters();

  const Font &font = *look.overlay_font;
  canvas.Select(font);
  FormatUserMapScale(map_width, buffer.buffer(), true);
  PixelSize text_size = canvas.CalcTextSize(buffer);

  // check if window too small to bother drawing
  if (text_size.width * 3 > rc.GetWidth())
    return rc.left;

  const int padding = int(Layout::GetTextPadding());
  const int height = int(GetMapScaleBandHeight(font));
  const int top = rc.bottom - height - 1;
  const int text_y = top + (height - int(font.GetHeight())) / 2;
  const unsigned icon_px = font.GetHeight();

  /* black text keeps a monochrome mark from inverting on the white pill */
  canvas.SetTextColor(COLOR_BLACK);

  const int left_w =
    look.map_scale_left_icon.GetScaledSize(icon_px).width;
  const int right_w =
    look.map_scale_right_icon.GetScaledSize(icon_px).width;
  const int icon_y = top + (height - int(icon_px)) / 2;

  PixelRect scale;
  scale.left = rc.left;
  scale.top = top;
  scale.bottom = top + height;
  const int left_slot = left_w + padding;
  const int right_slot = right_w + padding;
  scale.right = rc.left + left_slot + padding + (int)text_size.width
    + padding + right_slot;
  DrawPill(canvas, scale);

  look.map_scale_left_icon.Draw(canvas,
    {scale.left + (left_slot - left_w) / 2, icon_y}, icon_px);

  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();
  canvas.DrawText({scale.left + left_slot + padding, text_y}, buffer);

  look.map_scale_right_icon.Draw(canvas,
    {scale.right - right_slot + (right_slot - right_w) / 2, icon_y},
    icon_px);

  if (contour_spacing_m > 0) {
    const auto contour_buf = FormatUserAltitude((double)contour_spacing_m);
    const PixelSize contour_size = canvas.CalcTextSize(contour_buf.c_str());
    const int icon_width =
      look.contour_spacing_icon.GetScaledSize(icon_px).width;
    const int slot = icon_width + padding;

    PixelRect contour;
    contour.left = scale.right + padding * 2;
    contour.top = top;
    contour.bottom = top + height;
    contour.right = contour.left + slot + padding
      + (int)contour_size.width + padding;
    DrawPill(canvas, contour);

    canvas.SetTextColor(COLOR_BLACK);
    look.contour_spacing_icon.Draw(canvas,
      {contour.left + (slot - icon_width) / 2, icon_y}, icon_px);

    canvas.SetBackgroundTransparent();
    canvas.DrawText({contour.left + slot + padding, text_y},
                    contour_buf.c_str());
    return contour.right;
  }

  return scale.right;
}
