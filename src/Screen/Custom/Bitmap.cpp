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

#include "Screen/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "LibPNG.hpp"
#include "LibJPEG.hpp"
#include "UncompressedImage.hpp"
#include "Util/ConstBuffer.hpp"

Bitmap::Bitmap(ConstBuffer<void> _buffer)
  :
#ifdef ENABLE_OPENGL
#ifdef ANDROID
  id(0),
#endif
  texture(NULL), interpolation(false)
#else
  buffer(WritableImageBuffer<BitmapPixelTraits>::Empty())
#endif
{
  Load(_buffer);
}

bool
Bitmap::Load(ConstBuffer<void> buffer, Type type)
{
  const UncompressedImage uncompressed = LoadPNG(buffer.data, buffer.size);
  return Load(uncompressed, type);
}

bool
Bitmap::LoadFile(const TCHAR *path)
{
  const UncompressedImage uncompressed = LoadJPEGFile(path);
  return Load(uncompressed);
}
