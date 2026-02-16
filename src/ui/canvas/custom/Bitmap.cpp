// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "system/Path.hpp"

#ifdef ENABLE_COREGRAPHICS
#include "../apple/ImageDecoder.hpp"
#else
#include "LibPNG.hpp"
#include "LibJPEG.hpp"
#endif

#ifdef USE_LIBTIFF
#include "LibTiff.hpp"
#endif

#include "UncompressedImage.hpp"

#include <tchar.h>

Bitmap::Bitmap(std::span<const std::byte> _buffer)
{
  Load(_buffer);
}

bool
Bitmap::Load(std::span<const std::byte> buffer, Type type)
{
  auto uncompressed = LoadPNG(buffer);
  return uncompressed.IsDefined() && Load(std::move(uncompressed), type);
}

static UncompressedImage
DecompressImageFile(Path path)
{
#ifdef USE_LIBTIFF
  if (path.EndsWithIgnoreCase(".tif") || path.EndsWithIgnoreCase(".tiff"))
    return LoadTiff(path);
#endif

  if (path.EndsWithIgnoreCase(".png"))
    return LoadPNG(path);

  return LoadJPEGFile(path);
}

bool
Bitmap::LoadFile(Path path)
{
  auto uncompressed = DecompressImageFile(path);
  return uncompressed.IsDefined() && Load(std::move(uncompressed));
}
