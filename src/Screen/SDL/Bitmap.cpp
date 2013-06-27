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
#include "ResourceLoader.hpp"
#include "OS/ConvertPathName.hpp"
#include "Screen/SDL/Format.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Debug.hpp"
#else
#include "Screen/Custom/UncompressedImage.hpp"
#include "UncompressedImage.hpp"
#endif

#ifdef WIN32
  #include <windows.h>
#endif

#include <assert.h>

#ifndef ENABLE_OPENGL

bool
Bitmap::Load(const UncompressedImage &uncompressed, Type type)
{
  assert(IsScreenInitialized());
  assert(uncompressed.IsVisible());

  Reset();

  ImportSurface(buffer, uncompressed);
  return true;
}

#endif

bool
Bitmap::LoadStretch(unsigned id, unsigned zoom)
{
  assert(zoom > 0);

  // XXX
  return Load(id);
}

#ifndef ENABLE_OPENGL

void
Bitmap::Reset()
{
  assert(!IsDefined() || IsScreenInitialized());

  buffer.Free();
}

const PixelSize
Bitmap::GetSize() const
{
  assert(IsDefined());

  return { buffer.width, buffer.height };
}

#endif /* !OpenGL */
