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
               const OverlayLook &look,
               unsigned contour_spacing_m)
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

  const int top = rc.bottom - height - 1;

  /* The icons are scaled to the height of the white boxes so that the
     whole scale bar is one band, independent of DPI and font metrics.
     They are flush with the edge of their bitmap where they meet a
     box; overlap that seam by one pixel, because magnifying a texture
     can leave its outermost pixel column slightly translucent. */
  constexpr int overlap = 1;

  /* MaskedIcon::Draw() inverts greyscale icons when the canvas' text
     colour suggests a dark background.  These icons always sit on a
     white box, so pin the text colour before drawing them. */
  canvas.SetTextColor(COLOR_BLACK);

  int x = rc.left;
  look.map_scale_left_icon.Draw(canvas, PixelPoint(x, top), height);

  x += look.map_scale_left_icon.GetScaledSize(height).width;
  canvas.DrawFilledRectangle({{x - overlap, top},
                              PixelSize{overlap + 2 * text_padding_x + (int)text_size.width, height}}, COLOR_WHITE);

  canvas.SetBackgroundTransparent();
  x += text_padding_x;
  canvas.DrawText({x, rc.bottom - (int)(font.GetAscentHeight() + Layout::Scale(1u)) - 1},
                  buffer);

  x += text_padding_x + text_size.width;
  look.map_scale_right_icon.Draw(canvas, PixelPoint(x - overlap, top), height);

  if (contour_spacing_m > 0) {
    x += look.map_scale_right_icon.GetScaledSize(height).width;
    x += text_padding_x * 2;

    const auto contour_buf = FormatUserAltitude((double)contour_spacing_m);
    PixelSize contour_size = canvas.CalcTextSize(contour_buf.c_str());
    const int icon_width = look.contour_spacing_icon.GetScaledSize(height).width;

    canvas.DrawFilledRectangle(
      {{x, top},
       PixelSize{icon_width + 2 * text_padding_x + (int)contour_size.width, height}},
      COLOR_WHITE);

    look.contour_spacing_icon.Draw(canvas, PixelPoint(x, top), height);

    canvas.SetBackgroundTransparent();
    canvas.SetTextColor(COLOR_BLACK);
    canvas.DrawText(
      {x + icon_width + text_padding_x,
       rc.bottom - (int)(font.GetAscentHeight() + Layout::Scale(1u)) - 1},
      contour_buf.c_str());
  }
}
