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

#include "Screen/UnitSymbol.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "ResourceId.hpp"

#ifdef USE_MEMORY_CANVAS
#include "Debug.hpp"
#include "Custom/LibPNG.hpp"
#include "Custom/UncompressedImage.hpp"
#include "ResourceLoader.hpp"
#endif

#include <assert.h>

PixelSize
UnitSymbol::GetScreenSize() const
{
  PixelSize s = GetSize();
  return { Layout::Scale(s.cx), Layout::Scale(s.cy) };
}

void
UnitSymbol::Load(ResourceId id)
{
#ifdef USE_MEMORY_CANVAS
  assert(IsScreenInitialized());
  assert(buffer.data == nullptr);

  ResourceLoader::Data data = ResourceLoader::Load(id);
  assert(!data.IsNull());

  const UncompressedImage uncompressed = LoadPNG(data.data, data.size);
  assert(uncompressed.GetFormat() == UncompressedImage::Format::GRAY);

  const size_t size = uncompressed.GetPitch() * uncompressed.GetHeight();
  buffer.data = new Luminosity8[size];
  memcpy(buffer.data, uncompressed.GetData(), size);

  buffer.pitch = uncompressed.GetPitch();
  buffer.width = uncompressed.GetWidth();
  buffer.height = uncompressed.GetHeight();
#else
  bitmap.Load(id, Bitmap::Type::MONO);
  size = bitmap.GetSize();
#endif
}

void 
UnitSymbol::Draw(Canvas &canvas, PixelScalar x, PixelScalar y,
                 Color bg_color, Color text_color) const
{
  assert(IsDefined());

  const PixelSize size = GetSize();
  const PixelSize screen_size = GetScreenSize();
  canvas.StretchMono(x, y, screen_size.cx, screen_size.cy,
#if defined(USE_GDI) || defined(ENABLE_OPENGL)
                     bitmap,
#else
                     buffer,
#endif
                     0, 0, size.cx, size.cy,
                     text_color, bg_color);
}
