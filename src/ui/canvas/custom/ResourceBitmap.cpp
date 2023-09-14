// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "Screen/Debug.hpp"
#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

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
Bitmap::LoadStretch(ResourceId id, [[maybe_unused]] unsigned zoom)
{
  assert(zoom > 0);

  // XXX
  return Load(id);
}

#endif
