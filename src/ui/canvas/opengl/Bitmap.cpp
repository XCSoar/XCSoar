// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "UncompressedImage.hpp"
#include "Texture.hpp"
#include "Debug.hpp"

Bitmap::Bitmap(Bitmap &&src) noexcept
  :texture(std::exchange(src.texture, nullptr)),
   size(src.size),
   flipped(src.flipped)
{
}

Bitmap &Bitmap::operator=(Bitmap &&src) noexcept
{
  delete texture;
  texture = std::exchange(src.texture, nullptr);
  size = src.size;
  flipped = src.flipped;
  return *this;
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
