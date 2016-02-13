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

#include "Screen/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "OS/Path.hpp"

#ifdef ENABLE_COREGRAPHICS
#include "CoreGraphics.hpp"
#else
#include "LibPNG.hpp"
#include "LibJPEG.hpp"
#endif

#ifdef USE_LIBTIFF
#include "LibTiff.hpp"
#endif

#include "UncompressedImage.hpp"
#include "Util/ConstBuffer.hxx"

#include <tchar.h>

Bitmap::Bitmap(ConstBuffer<void> _buffer)
{
  Load(_buffer);
}

bool
Bitmap::Load(ConstBuffer<void> buffer, Type type)
{
  auto uncompressed = LoadPNG(buffer.data, buffer.size);
  return uncompressed.IsDefined() && Load(std::move(uncompressed), type);
}

static UncompressedImage
DecompressImageFile(Path path)
{
#ifdef USE_LIBTIFF
  if (path.MatchesExtension(_T(".tif")) || path.MatchesExtension(_T(".tiff")))
    return LoadTiff(path);
#endif

  if (path.MatchesExtension(_T(".png")))
    return LoadPNG(path);

  return LoadJPEGFile(path);
}

bool
Bitmap::LoadFile(Path path)
{
  auto uncompressed = DecompressImageFile(path);
  return uncompressed.IsDefined() && Load(std::move(uncompressed));
}
