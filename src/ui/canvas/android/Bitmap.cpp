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
#include "ui/canvas/opengl/Texture.hpp"
#include "Android/Bitmap.hpp"
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#include "system/Path.hpp"
#include "ResourceId.hpp"
#include "android_drawable.h"

Bitmap::Bitmap(ResourceId id)
{
  Load(id);
}

static const char *
find_resource_name(unsigned id) noexcept
{
  for (unsigned i = 0; DrawableNames[i].name != nullptr; ++i)
    if (DrawableNames[i].id == id)
      return DrawableNames[i].name;

  return nullptr;
}

static Java::LocalObject
LoadResourceBitmap(ResourceId id)
{
  const char *name = find_resource_name((unsigned)id);
  if (name == nullptr)
    return nullptr;

  return native_view->loadResourceBitmap(name);
}

bool
Bitmap::Set(JNIEnv *env, jobject bmp, Type type, bool flipped) noexcept
{
  assert(bmp != nullptr);

  size.width = AndroidBitmap::GetWidth(env, bmp);
  size.height = AndroidBitmap::GetHeight(env, bmp);

  if (!MakeTexture(bmp, type, flipped)) {
    Reset();
    return false;
  }

  return true;
}

bool
Bitmap::MakeTexture(jobject _bmp, Type _type, bool flipped) noexcept
{
  assert(_bmp != nullptr);

  jint result[5];
  if (!native_view->bitmapToTexture(_bmp, _type == Bitmap::Type::MONO, result))
    return false;

  texture = new GLTexture(result[0], PixelSize(result[1], result[2]),
                          PixelSize(result[3], result[4]), flipped);
  if (interpolation) {
    texture->Bind();
    texture->EnableInterpolation();
  }

  return true;
}

bool
Bitmap::Load(ResourceId id, Type _type)
{
  assert(id.IsDefined());

  Reset();

  auto new_bmp = LoadResourceBitmap(id);
  if (new_bmp == nullptr)
    return false;

  return Set(new_bmp.GetEnv(), new_bmp, _type);
}

bool
Bitmap::LoadFile(Path path)
{
  assert(path != nullptr && !path.IsEmpty());

  Reset();

  Java::LocalObject new_bmp;
  bool flipped = false;
  if (path.MatchesExtension(_T(".tif")) || path.MatchesExtension(_T(".tiff"))) {
    new_bmp = native_view->loadFileTiff(path);
    flipped = true;
  } else {
    new_bmp = native_view->loadFileBitmap(path);
  }
  if (new_bmp == nullptr)
    return false;

  return Set(Java::GetEnv(), new_bmp, Type::STANDARD, flipped);
}
