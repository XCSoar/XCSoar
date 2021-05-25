/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "LogoView.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/FontDescription.hpp"
#include "Resources.hpp"
#include "Version.hpp"

#include <algorithm>

LogoView::LogoView() noexcept try
  :logo(IDB_LOGO), big_logo(IDB_LOGO_HD),
   title(IDB_TITLE), big_title(IDB_TITLE_HD)
{
#ifndef USE_GDI
  font.Load(FontDescription(Layout::FontScale(10)));
#endif

  big_logo.EnableInterpolation();
  big_title.EnableInterpolation();
} catch (...) {
  /* ignore Bitmap/Font loader exceptions */
}

static int
Center(unsigned canvas_size, unsigned element_size)
{
  /* cast to int to force signed integer division, just in case the
     difference is negative */
  return int(canvas_size - element_size) / 2;
}

void
LogoView::draw(Canvas &canvas, const PixelRect &rc) noexcept
{
  if (!big_logo.IsDefined() || !big_title.IsDefined())
    return;

  const unsigned width = rc.GetWidth(), height = rc.GetHeight();

  enum {
    LANDSCAPE, PORTRAIT, SQUARE,
  } orientation;

  if (width == height)
    orientation = SQUARE;
  else if (width > height)
    orientation = LANDSCAPE;
  else
    orientation = PORTRAIT;

  /* load bitmaps */
  const bool use_big =
    (orientation == LANDSCAPE && width >= 510 && height >= 170) ||
    (orientation == PORTRAIT && width >= 330 && height >= 250) ||
    (orientation == SQUARE && width >= 210 && height >= 210);
  const Bitmap &bitmap_logo = use_big ? big_logo : logo;
  const Bitmap &bitmap_title = use_big ? big_title : title;

  // Determine logo size
  PixelSize logo_size = bitmap_logo.GetSize();

  // Determine title image size
  PixelSize title_size = bitmap_title.GetSize();

  unsigned spacing = title_size.height / 2;

  unsigned estimated_width, estimated_height;
  switch (orientation) {
  case LANDSCAPE:
    estimated_width = logo_size.width + spacing + title_size.width;
    estimated_height = logo_size.height;
    break;

  case PORTRAIT:
    estimated_width = title_size.width;
    estimated_height = logo_size.height + spacing + title_size.height;
    break;

  case SQUARE:
    estimated_width = logo_size.width;
    estimated_height = logo_size.height;
    break;
  }

  const unsigned magnification =
    std::min((width - 16u) / estimated_width,
             (height - 16u) / estimated_height);

  if (magnification > 1) {
    logo_size.width *= magnification;
    logo_size.height *= magnification;
    title_size.width *= magnification;
    title_size.height *= magnification;
    spacing *= magnification;
  }

  PixelPoint logo_position, title_position;

  // Determine logo and title positions
  switch (orientation) {
  case LANDSCAPE:
    logo_position.x = Center(width, logo_size.width + spacing + title_size.width);
    logo_position.y = Center(height, logo_size.height);
    title_position.x = logo_position.x + logo_size.width + spacing;
    title_position.y = Center(height, title_size.height);
    break;
  case PORTRAIT:
    logo_position.x = Center(width, logo_size.width);
    logo_position.y = Center(height, logo_size.height + spacing + title_size.height);
    title_position.x = Center(width, title_size.width);
    title_position.y = logo_position.y + logo_size.height + spacing;
    break;
  case SQUARE:
    logo_position.x = Center(width, logo_size.width);
    logo_position.y = Center(height, logo_size.height);
    // not needed - silence compiler "may be used uninitialized"
    title_position.x = 0;
    title_position.y = 0;
    break;
  }

  // Draw 'XCSoar N.N' title
  if (orientation != SQUARE)
    canvas.Stretch(title_position, title_size, bitmap_title);

  // Draw XCSoar swift logo
  canvas.Stretch(logo_position, logo_size, bitmap_logo);

  // Draw full XCSoar version number

#ifndef USE_GDI
  if (!font.IsDefined())
    return;

  canvas.Select(font);
#endif

  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();
  canvas.DrawText({2, 2}, XCSoar_ProductToken);
}
