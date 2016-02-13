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
  if (data.IsNull())
    return false;

  return Load(data, type);
}

#ifdef USE_MEMORY_CANVAS

bool
Bitmap::LoadStretch(ResourceId id, unsigned zoom)
{
  assert(zoom > 0);

  // XXX
  return Load(id);
}

#endif
