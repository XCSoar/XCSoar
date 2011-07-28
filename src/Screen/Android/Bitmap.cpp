/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

bool
Bitmap::load(unsigned _id)
{
  id = _id;

  reset();

  if (!surface_valid) {
    AddSurfaceListener(*this);
    return true;
  }

  texture = LoadResourceTexture(id);
  if (texture == NULL)
    return false;

  width = texture->get_width();
  height = texture->get_height();

  AddSurfaceListener(*this);

  return true;
}

bool
Bitmap::load_stretch(unsigned id, unsigned zoom)
{
  assert(zoom > 0);

  // XXX
  return load(id);
}

bool
Bitmap::load_file(const TCHAR *path)
{
  // XXX
  return false;
}

void
Bitmap::reset()
{
  if (texture == NULL)
    return;

  RemoveSurfaceListener(*this);

  delete texture;
  texture = NULL;
}

const PixelSize
Bitmap::get_size() const
{
  assert(defined());

  const PixelSize size = { width, height };
  return size;
}
