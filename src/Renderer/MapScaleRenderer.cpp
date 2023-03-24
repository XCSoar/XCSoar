// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapScaleRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Projection/WindowProjection.hpp"
#include "Look/OverlayLook.hpp"
#include "util/StaticString.hxx"
#include "Formatter/UserUnits.hpp"

void
RenderMapScale(Canvas &canvas,
               const WindowProjection& projection,
               const PixelRect &rc,
               const OverlayLook &look)
{
  if (!projection.IsValid())
    return;

  StaticString<80> buffer;

  auto map_width = projection.GetScreenWidthMeters();

  const Font &font = *look.overlay_font;
  canvas.Select(font);
  FormatUserMapScale(map_width, buffer.buffer(), true);
  PixelSize text_size = canvas.CalcTextSize(buffer);

  // check if window too small to bother drawing
  if (text_size.width * 3 > rc.GetWidth())
    return;

  const int text_padding_x = Layout::GetTextPadding();
  const int height = font.GetCapitalHeight()
      + Layout::GetTextPadding();

  int x = rc.left;
  look.map_scale_left_icon.Draw(canvas, PixelPoint(x, rc.bottom - height));

  x += look.map_scale_left_icon.GetSize().width;
  canvas.DrawFilledRectangle({{x, rc.bottom - height},
                              PixelSize{2 * text_padding_x + (int)text_size.width, height}}, COLOR_WHITE);

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);
  x += text_padding_x;
  canvas.DrawText({x, rc.bottom - (int)(font.GetAscentHeight() + Layout::Scale(1u))},
                  buffer);

  x += text_padding_x + text_size.width;
  look.map_scale_right_icon.Draw(canvas, PixelPoint(x, rc.bottom - height));
}
