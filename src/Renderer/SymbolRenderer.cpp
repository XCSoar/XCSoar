/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "SymbolRenderer.hpp"
#include "Screen/Canvas.hpp"

#include <algorithm>

void
SymbolRenderer::DrawSign(Canvas &canvas, PixelRect rc, bool plus)
{
  PixelScalar size = std::min(rc.right - rc.left, rc.bottom - rc.top) / 5;
  RasterPoint center = rc.GetCenter();

  // Draw horizontal bar
  canvas.Rectangle(center.x - size, center.y - size / 3,
                   center.x + size, center.y + size / 3);

  if (plus)
    // Draw vertical bar
    canvas.Rectangle(center.x - size / 3, center.y - size,
                     center.x + size / 3, center.y + size);
}
