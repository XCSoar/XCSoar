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

#include "ui/canvas/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "UncompressedImage.hpp"
#include "Texture.hpp"
#include "Debug.hpp"

Bitmap::Bitmap(Bitmap &&src) noexcept
  :texture(std::exchange(src.texture, nullptr)),
   size(src.size),
   interpolation(src.interpolation),
   flipped(src.flipped)
{
}

Bitmap &Bitmap::operator=(Bitmap &&src) noexcept
{
  delete texture;
  texture = std::exchange(src.texture, nullptr);
  size = src.size;
  interpolation = src.interpolation;
  flipped = src.flipped;
  return *this;
}

void
Bitmap::EnableInterpolation() noexcept
{
  interpolation = true;
  if (texture != nullptr) {
    texture->Bind();
    texture->EnableInterpolation();
  }
}

bool
Bitmap::MakeTexture(const UncompressedImage &uncompressed, Type type) noexcept
{
  assert(IsScreenInitialized());
  assert(uncompressed.IsDefined());

  texture = type == Type::MONO
    ? ImportAlphaTexture(uncompressed)
    : ImportTexture(uncompressed);
  if (texture == nullptr)
    return false;

  if (interpolation)
    texture->EnableInterpolation();

  return true;
}

bool
Bitmap::Load(UncompressedImage &&_uncompressed, Type _type)
{
  assert(IsScreenInitialized());
  assert(_uncompressed.IsDefined());

  Reset();

  size = { _uncompressed.GetWidth(), _uncompressed.GetHeight() };
  flipped = _uncompressed.IsFlipped();

  if (!MakeTexture(_uncompressed, _type)) {
    Reset();
    return false;
  }

  return true;
}

void
Bitmap::Reset() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());
  assert(!IsDefined() || pthread_equal(pthread_self(), OpenGL::thread));

  delete texture;
  texture = nullptr;
}
