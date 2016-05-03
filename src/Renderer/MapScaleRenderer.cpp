/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  }
*/

#include "MapScaleRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Projection/WindowProjection.hpp"
#include "Look/OverlayLook.hpp"
#include "Util/StaticString.hxx"
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
  if ((unsigned)text_size.cx*3 > rc.GetWidth())
    return;

  const int text_padding_x = Layout::GetTextPadding();
  const unsigned height = font.GetCapitalHeight()
      + Layout::GetTextPadding();

  int x = rc.left;
  look.map_scale_left_icon.Draw(canvas, PixelPoint(x, rc.bottom - height));

  x += look.map_scale_left_icon.GetSize().cx;
  canvas.DrawFilledRectangle(x, rc.bottom - height,
                             x + 2 * text_padding_x + text_size.cx,
                             rc.bottom, COLOR_WHITE);

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);
  x += text_padding_x;
  canvas.DrawText(x,
                  rc.bottom - font.GetAscentHeight() - Layout::Scale(1),
                  buffer);

  x += text_padding_x + text_size.cx;
  look.map_scale_right_icon.Draw(canvas, PixelPoint(x, rc.bottom - height));
}
