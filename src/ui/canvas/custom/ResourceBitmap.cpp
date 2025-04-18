// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

#ifdef USE_MEMORY_CANVAS
#ifdef __ARM_NEON__
#include "ui/canvas/memory/NEON.hpp"
#endif
#include "ui/canvas/memory/RasterCanvas.hpp"
#include "ui/canvas/memory/PixelOperations.hpp"
#endif

#ifdef ENABLE_OPENGL

Bitmap::Bitmap(ResourceId id)
{
  Load(id);
}

#endif

#ifdef USE_MEMORY_CANVAS

Bitmap::Bitmap(ResourceId id)
{
  Load(id);
}

#endif

bool
Bitmap::Load(ResourceId id, Type type)
{
  assert(IsScreenInitialized());

  ResourceLoader::Data data = ResourceLoader::Load(id);
  if (data.data() == nullptr)
    return false;

  return Load(data, type);
}

#ifdef USE_MEMORY_CANVAS

bool
Bitmap::LoadStretch(ResourceId id, unsigned zoom)
{
  assert(zoom > 0);

  if (!Load(id))
    return false;

  Bitmap stretched;
  stretched.Create(GetSize() * zoom);

  {
    RasterCanvas canvas{stretched.buffer};
    canvas.ScaleRectangle({}, stretched.GetSize(),
                          buffer.data, buffer.pitch, buffer.size);
  }

  *this = std::move(stretched);
  return true;
}

#endif
