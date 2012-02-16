/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Screen/UnitSymbol.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>

const RasterPoint
UnitSymbol::get_origin(enum kind kind) const
{
  assert(kind >= 0 && kind < 4);

  RasterPoint origin;
  origin.x = size.cx * kind;
  origin.y = 0;
  return origin;
}

PixelSize
UnitSymbol::GetScreenSize() const
{
  return PixelSize{ PixelScalar(Layout::Scale(size.cx)),
      PixelScalar(Layout::Scale(size.cy)) };
}

void 
UnitSymbol::draw(Canvas &canvas, PixelScalar x, PixelScalar y, kind k) const
{
  const RasterPoint BmpPos = get_origin(k);
  const PixelSize size = get_size();
  canvas.scale_copy(x, y, bitmap,
		    BmpPos.x, BmpPos.y,
		    size.cx, size.cy);
}
