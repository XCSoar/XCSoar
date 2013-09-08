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
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Surface.hpp"
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#include "android_drawable.h"

static const char *
find_resource_name(unsigned id)
{
  for (unsigned i = 0; DrawableNames[i].name != NULL; ++i)
    if (DrawableNames[i].id == id)
      return DrawableNames[i].name;

  return NULL;
}

gcc_malloc
static GLTexture *
LoadResourceTexture(unsigned id)
{
  const char *name = find_resource_name(id);
  if (name == NULL)
    return NULL;

  jint result[3];
  if (!native_view->loadResourceTexture(name, result))
    return NULL;

  return new GLTexture(result[0], result[1], result[2]);
}

gcc_malloc
static GLTexture *
LoadFileTexture(const TCHAR *path)
{
  jint result[3];
  if (!native_view->loadFileTexture(path, result))
    return NULL;

  return new GLTexture(result[0], result[1], result[2]);
}

bool
Bitmap::Reload()
{
  assert(id != 0 || !pathName.empty());
  assert(texture == NULL);

  texture = (id != 0) ? LoadResourceTexture(id) :
                        LoadFileTexture(pathName.c_str());
  if (texture == NULL)
    return false;

  size.cx = texture->GetWidth();
  size.cy = texture->GetHeight();
  return true;
}

bool
Bitmap::Load(unsigned _id, Type type)
{
  assert(_id != 0);

  Reset();

  id = _id;
  AddSurfaceListener(*this);

  if (!surface_valid)
    return true;

  return Reload();
}

bool
Bitmap::LoadFile(const TCHAR *path)
{
  assert(path != NULL && *path != _T('\0'));

  Reset();

  pathName = path;
  AddSurfaceListener(*this);

  if (!surface_valid)
    return true;

  return Reload();
}

void
Bitmap::Reset()
{
  if (id != 0 || !pathName.empty()) {
    RemoveSurfaceListener(*this);
    id = 0;
    pathName.clear();
  }

  delete texture;
  texture = NULL;
}

void
Bitmap::SurfaceCreated()
{
  Reload();
}

void
Bitmap::SurfaceDestroyed()
{
  delete texture;
  texture = NULL;
}
